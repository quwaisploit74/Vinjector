package main

import (
        "flag"
        "fmt"
        "math"
        "os"
        "os/exec"
        "path/filepath"
        "sort"
        "strconv"
        "strings"
        "time"
)

// Hacker Palette (Red, Blue, White Focus)
const (
        R     = "\033[31m"   // Red
        B     = "\033[34m"   // Blue
        W     = "\033[37m"   // White
        BOLD  = "\033[1m"
        RESET = "\033[0m"
)

type Config struct {
        gifPath      string
        iconPath     string
        videoPath    string
        duration     int
        outputFolder string
        deleteSource bool
        minSize      float64
}

type Vinjector struct {
        cfg         *Config
        ffmpegPath  string
        ffprobePath string
}

func NewVinjector(cfg *Config) *Vinjector {
        return &Vinjector{
                cfg:         cfg,
                ffmpegPath:  "ffmpeg",
                ffprobePath: "ffprobe",
        }
}

func (v *Vinjector) log(tag, msg, color string) {
        fmt.Printf("%s[%s%s%s%s%s] %s%s\n", W, color, BOLD, tag, RESET, W, msg, RESET)
}

func (v *Vinjector) getVideoDuration(path string) float64 {
        cmd := exec.Command(v.ffprobePath, "-v", "error", "-show_entries",
                "format=duration", "-of", "default=noprint_wrappers=1:nokey=1", path)
        out, err := cmd.Output()
        if err != nil {
                return 0
        }
        dur, _ := strconv.ParseFloat(strings.TrimSpace(string(out)), 64)
        return dur
}

func (v *Vinjector) showSyringeBanner() {
        // ASCII Art: Syringe (Suntikan) - Red & White
        banner := []string{
                R + BOLD + "      💉 VINJECTOR v2.6.7-BUILD [codename: hacker]" + RESET,
                W + "                _________________________________________",
                W + "  _____________/                                         " + B + "\\",
                R + "  [############" + W + "             " + B + "   BY: M. QUWAIS SAPUTRA   " + B + " |---" + RESET,
                W + "  ¯¯¯¯¯¯¯¯¯¯¯¯¯\\________________________________________/" + RESET,
                "",
        }

        for _, line := range banner {
                fmt.Println(line)
                time.Sleep(30 * time.Millisecond)
        }
}

func (v *Vinjector) execute() {
        if _, err := os.Stat(v.cfg.videoPath); os.IsNotExist(err) {
                v.log("ERR", "Target video not found!", R)
                return
        }

        totalDur := v.getVideoDuration(v.cfg.videoPath)
        if totalDur == 0 {
                v.log("ERR", "FFmpeg failed to read metadata!", R)
                return
        }

        estFrags := int(math.Ceil(totalDur / float64(v.cfg.duration)))
        v.log("SYS", fmt.Sprintf("Source: %s", filepath.Base(v.cfg.videoPath)), B)
        v.log("SYS", fmt.Sprintf("Duration: %ds | Sectors: %d", int(totalDur), estFrags), B)

        if v.cfg.minSize > 0 {
                v.log("INF", fmt.Sprintf("Min Size Guard: %.2f MB", v.cfg.minSize), W)
        }
        fmt.Println(R + strings.Repeat("—", 50) + RESET)

        os.MkdirAll(v.cfg.outputFolder, 0755)

        v.log("EXEC", "Fragmenting binary stream...", B)
        splitCmd := exec.Command(v.ffmpegPath, "-v", "error", "-i", v.cfg.videoPath,
                "-f", "segment", "-segment_time", strconv.Itoa(v.cfg.duration),
                "-c", "copy", "-reset_timestamps", "1", "chunk_%03d.mp4", "-y")
        splitCmd.Run()

        matches, _ := filepath.Glob("chunk_*.mp4")
        sort.Strings(matches)

        totalStart := time.Now()
        for i, frag := range matches {
                out := filepath.Join(v.cfg.outputFolder, fmt.Sprintf("payload_%d.mp4", i+1))

                etaStr := ""
                if i > 0 {
                        elapsed := time.Since(totalStart).Seconds()
                        remaining := (elapsed / float64(i)) * float64(len(matches)-i)
                        etaStr = fmt.Sprintf(" | ETA: %ds", int(remaining))
                }

                fmt.Printf("\r%s[%sLOADING %d/%d%s]%s%s%s", W, R, i+1, len(matches), W, B, etaStr, RESET)

                // Filter Complex for Injection
                vFilter := "[0:v]scale=-2:400,pad=720:480:(720-iw)/2:40,setsar=1[m];" +
                        "[1:v]scale=720:40[t];[2:v]scale=720:40[b];" +
                        "[3:v]scale=100:-1,format=rgba,colorchannelmixer=aa=0.8[l];" +
                        "[m][t]overlay=0:0:shortest=1[v1];[v1][b]overlay=0:H-h:shortest=1[v2];[v2][l]overlay=20:50"

                renderCmd := exec.Command(v.ffmpegPath, "-v", "error", "-i", frag,
                        "-ignore_loop", "0", "-i", v.cfg.gifPath, "-ignore_loop", "0", "-i", v.cfg.gifPath,
                        "-i", v.cfg.iconPath, "-filter_complex", vFilter, "-map", "0:a?", "-c:v", "libx264",
                        "-b:v", "600k", "-preset", "superfast", "-shortest", out, "-y")

                renderCmd.Run()
                os.Remove(frag)

                // File Size Filter
                if v.cfg.minSize > 0 {
                        if f, err := os.Stat(out); err == nil {
                                if float64(f.Size())/(1024*1024) < v.cfg.minSize {
                                        os.Remove(out)
                                }
                        }
                }
        }

        fmt.Printf("\n\n%s%s[COMPLETE] PAYLOAD DEPLOYED TO /%s%s\n", R, BOLD, v.cfg.outputFolder, RESET)
        if v.cfg.deleteSource {
                os.Remove(v.cfg.videoPath)
                v.log("SYS", "Source wiped from disk.", R)
        }
}

func main() {
        cfg := &Config{}

        flag.StringVar(&cfg.videoPath, "p", "", "Target video path")
        flag.StringVar(&cfg.gifPath, "f", "", "Overlay GIF path")
        flag.StringVar(&cfg.iconPath, "i", "", "PNG Icon path")
        flag.IntVar(&cfg.duration, "d", 0, "Split duration (sec)")
        flag.BoolVar(&cfg.deleteSource, "x", false, "Wipe source after injection")
        flag.StringVar(&cfg.outputFolder, "o", "injected_outputs", "Output directory")
        flag.Float64Var(&cfg.minSize, "m", 0, "Auto-delete if size < MB")

        flag.Parse()

        if cfg.videoPath == "" || cfg.gifPath == "" || cfg.iconPath == "" || cfg.duration == 0 {
                fmt.Printf("%s%sVINJECTOR v2.6.7-BUILD%s\n", BOLD, R, RESET)
                fmt.Println("Usage: ./vinjector -p <vid> -f <gif> -i <png> -d <sec> [-m 8.0] [-x]")
                return
        }

        app := NewVinjector(cfg)
        app.showSyringeBanner()
        app.execute()
}
