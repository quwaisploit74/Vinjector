package main

import (
	"bufio"
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

// Hacker Palette
const (
	R     = "\u001B[31m"
	G     = "\u001B[32m"
	Y     = "\u001B[33m"
	B     = "\u001B[34m"
	M     = "\u001B[35m"
	C     = "\u001B[36m"
	W     = "\u001B[37m"
	BOLD  = "\u001B[1m"
	RESET = "\u001B[0m"
)

type Config struct {
	gifPath      string
	iconPath     string
	videoPath    string
	duration     int
	outputFolder string
	deleteSource bool
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

func (v *Vinjector) showBugBanner() {
	bugLines := []string{
		C + "  * .  * .  * .  * .  * .  * .  * .  * .  *",
		W + "          __      " + G + "  ________________________",
		W + "        .d$$b     " + G + " < " + W + BOLD + "VINJECTOR Go-v2.6 " + G + ">",
		W + "      .' TO$;\\    " + G + "  ------------------------",
		W + "     /  : TP._;   " + C + "  * .  * .  * .  * .  *",
		W + "    / _.;  :Tb|   " + M + "  TARGET: " + W + "Binary Injection",
		W + "   /   /   ;j$j   " + M + "  AUTHOR: " + W + "M. Quwais Saputra",
		W + "  _.-'       d$$$$",
		W + ".' ..       d$$$$; " + G + BOLD + "[ SYSTEM READY ]",
	}

	for _, line := range bugLines {
		fmt.Println(line)
		time.Sleep(20 * time.Millisecond)
	}
	fmt.Println()
}

func (v *Vinjector) execute() {
	// Check file
	if _, err := os.Stat(v.cfg.videoPath); os.IsNotExist(err) {
		v.log("FAIL", "Target missing!", R)
		return
	}

	totalDur := v.getVideoDuration(v.cfg.videoPath)
	if totalDur == 0 {
		v.log("FAIL", "Cannot read video duration. Check FFmpeg!", R);
		return
	}

	estFrags := int(math.Ceil(totalDur / float64(v.cfg.duration)))
	v.log("SCAN", fmt.Sprintf("Total duration: %ds", int(totalDur)), C)
	v.log("SCAN", fmt.Sprintf("Estimated sectors: %d", estFrags), C)
	fmt.Println(G + strings.Repeat("=", 55) + RESET)

	// Create output folder
	os.MkdirAll(v.cfg.outputFolder, 0755)

	v.log("INFO", "Splitting source...", Y)
	splitCmd := exec.Command(v.ffmpegPath, "-v", "error", "-i", v.cfg.videoPath,
		"-f", "segment", "-segment_time", strconv.Itoa(v.cfg.duration),
		"-c", "copy", "-reset_timestamps", "1", "temp_h_%03d.mp4", "-y")
	splitCmd.Run()

	// Get split files
	matches, _ := filepath.Glob("temp_h_*.mp4")
	sort.Strings(matches)

	totalStart := time.Now()
	for i, frag := range matches {
		out := filepath.Join(v.cfg.outputFolder, fmt.Sprintf("inj_%d.mp4", i+1))
		
		etaStr := ""
		if i > 0 {
			elapsed := time.Since(totalStart).Seconds()
			remaining := (elapsed / float64(i)) * float64(len(matches)-i)
			etaStr = fmt.Sprintf(" | ETA: %ds", int(remaining))
		}

		fmt.Printf("\r%s[INJECTING %d/%d]%s%s%s    %s", C, i+1, len(matches), RESET, Y, etaStr, RESET)

		vFilter := "[0:v]scale=-2:400,pad=720:480:(720-iw)/2:40,setsar=1[m];" +
			"[1:v]scale=720:40[t];[2:v]scale=720:40[b];" +
			"[3:v]scale=100:-1,format=rgba,colorchannelmixer=aa=0.8[l];" +
			"[m][t]overlay=0:0:shortest=1[v1];[v1][b]overlay=0:H-h:shortest=1[v2];[v2][l]overlay=20:50"

		renderCmd := exec.Command(v.ffmpegPath, "-v", "error", "-i", frag,
			"-ignore_loop", "0", "-i", v.cfg.gifPath, "-ignore_loop", "0", "-i", v.cfg.gifPath,
			"-i", v.cfg.iconPath, "-filter_complex", vFilter, "-map", "0:a?", "-c:v", "libx264",
			"-b:v", "600k", "-preset", "superfast", "-shortest", out, "-y")

		renderCmd.Run()
		os.Remove(frag) // Clean temp
	}

	fmt.Printf("\n\n%s%s[SUCCESS] BUG PAYLOAD INJECTED!%s\n", G, BOLD, RESET)
	if v.cfg.deleteSource {
		os.Remove(v.cfg.videoPath)
	}
}

func main() {
	cfg := &Config{}
	
	// Parsing flags
	flag.StringVar(&cfg.videoPath, "p", "", "Path video target")
	flag.StringVar(&cfg.gifPath, "f", "", "Path GIF overlay")
	flag.StringVar(&cfg.iconPath, "i", "", "Path Icon PNG")
	flag.IntVar(&cfg.duration, "d", 0, "Duration per split")
	flag.BoolVar(&cfg.deleteSource, "x", false, "Hapus sumber setelah selesai")
	flag.StringVar(&cfg.outputFolder, "o", "video", "Folder output")
	
	flag.Parse()

	if cfg.videoPath == "" || cfg.gifPath == "" || cfg.iconPath == "" || cfg.duration == 0 {
		fmt.Printf("%s%sVinjector Go Edition v2.6%s\n", BOLD, G, RESET)
		fmt.Println("Usage: ./vinjector -p <vid> -f <gif> -i <png> -d <sec> [-x]")
		return
	}

	app := NewVinjector(cfg)
	app.showBugBanner()
	app.execute()
}
