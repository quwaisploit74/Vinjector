import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.stream.Collectors;

public class Vinjector {
    // Hacker Palette
    public static final String R = "\u001B[31m";    public static final String G = "\u001B[32m";
    public static final String Y = "\u001B[33m";    public static final String B = "\u001B[34m";
    public static final String M = "\u001B[35m";    public static final String C = "\u001B[36m";
    public static final String W = "\u001B[37m";    public static final String BOLD = "\u001B[1m";
    public static final String RESET = "\u001B[0m"; public static final String BLINK = "\u001B[5m";

    private String ffmpegPath = "ffmpeg"; 
    private String ffprobePath = "ffprobe";

    static class Config {
        String gifPath, iconPath, videoPath;
        int duration = 0;
        String outputFolder = "video";
        double minSizeMb = 0.0;
        boolean deleteSource = false;
    }

    private final Config cfg;

    public Vinjector(Config cfg) {
        this.cfg = cfg;
    }

    private void log(String tag, String msg, String color) {
        System.out.println(W + "[" + color + BOLD + tag + RESET + W + "] " + msg + RESET);
    }

    private void prepareBinaries() {
        String os = System.getProperty("os.name").toLowerCase();
        String ext = os.contains("win") ? ".exe" : "";
        
        try {
            ffmpegPath = extractResource("/resources/ffmpeg" + ext);
            ffprobePath = extractResource("/resources/ffprobe" + ext);
            
            File f1 = new File(ffmpegPath);
            File f2 = new File(ffprobePath);
            
            if (!os.contains("win")) {
                f1.setExecutable(true);
                f2.setExecutable(true);
            }
        } catch (Exception e) {
            log("WARN", "Internal binaries missing, using system FFmpeg.", Y);
            ffmpegPath = "ffmpeg";
            ffprobePath = "ffprobe";
        }
    }

    private String extractResource(String resPath) throws Exception {
        InputStream is = getClass().getResourceAsStream(resPath);
        if (is == null) throw new FileNotFoundException("Resource not found: " + resPath);
        
        Path tempDir = Paths.get(System.getProperty("java.io.tmpdir"), "vinjector_bin");
        Files.createDirectories(tempDir);
        
        File targetFile = new File(tempDir.toFile(), resPath.substring(resPath.lastIndexOf("/") + 1));
        Files.copy(is, targetFile.toPath(), StandardCopyOption.REPLACE_EXISTING);
        return targetFile.getAbsolutePath();
    }

    private double getVideoDuration(String path) {
        try {
            ProcessBuilder pb = new ProcessBuilder(ffprobePath, "-v", "error", "-show_entries",
                "format=duration", "-of", "default=noprint_wrappers=1:nokey=1", path);
            Process p = pb.start();
            BufferedReader reader = new BufferedReader(new InputStreamReader(p.getInputStream()));
            String line = reader.readLine();
            return (line != null) ? Double.parseDouble(line) : 0;
        } catch (Exception e) { return 0; }
    }

    public void showBugBanner() {
        String[] bugLines = {
            C + "  * .  * .  * .  * .  * .  * .  * .  * .  *",
            W + "          __      " + G + "  ________________________",
            W + "        .d$$b     " + G + " < " + W + BOLD + "VINJECTOR v2.6-BUG " + G + ">",
            W + "      .' TO$;\\    " + G + "  ------------------------",
            W + "     /  : TP._;   " + C + "  * .  * .  * .  * .  *",
            W + "    / _.;  :Tb|   " + M + "  TARGET: " + W + "Binary Injection",
            W + "   /   /   ;j$j   " + M + "  AUTHOR: " + W + "M. Quwais Saputra",
            W + "  _.-'       d$$$$",
            W + ".' ..       d$$$$; " + G + BOLD + "[ SYSTEM READY ]",
            W + "/  /P'      d$$$$P. |\\",
            W + "/   \"      .d$$$P' |\\^\"l",
            W + ".'           `T$P^\"\"\"\"\"  :",
            W + "._.'      _.'                ;",
            W + "`-.-\".-'-' ._.       _.-\"    .-\"",
            W + "`.-\" _____  ._              .-\"",
            W + "-(.g$$$$$$$b.              .'",
            W + "  \"\"^^T$$$P^)            .(:",
            W + "    _/  -\"  /.'         /:/;",
            W + " ._.'-'`-'  \")/         /;/;",
            W + "`-.-\"..--\"\"   \" /         /  ;",
            W + ".-\" ..--\"\"        -'          :",
            W + "..--\"\"--.-\"         (\\      .-(\\        " + C + "*",
            W + "  ..--\"\"              `-\\(\\/;`         " + C + ". *",
            W + "    _.                      :            " + C + ".",
            W + "                            ;" + R + "`-",
            W + "                           :" + R + "\\",
            W + "                           ;" + R + "  bug" + RESET
        };

        for (String line : bugLines) {
            System.out.println(line);
            try { Thread.sleep(20); } catch (Exception e) {}
        }
        System.out.println();
    }

    public void execute() throws Exception {
        prepareBinaries();
        File videoFile = new File(cfg.videoPath);
        if (!videoFile.exists()) {
            log("FAIL", "Target missing!", R);
            return;
        }

        double totalDur = getVideoDuration(cfg.videoPath);
        if (totalDur == 0) {
            log("FAIL", "Cannot read video duration. Check FFmpeg!", R);
            return;
        }

        int estFrags = (int) Math.ceil(totalDur / cfg.duration);
        log("SCAN", "Total duration: " + (int) totalDur + "s", C);
        log("SCAN", "Estimated sectors: " + estFrags, C);
        System.out.println(G + "=".repeat(55) + RESET);

        Files.createDirectories(Paths.get(cfg.outputFolder));

        log("INFO", "Splitting source...", Y);
        ProcessBuilder splitPb = new ProcessBuilder(ffmpegPath, "-v", "error", "-i", cfg.videoPath,
            "-f", "segment", "-segment_time", String.valueOf(cfg.duration),
            "-c", "copy", "-reset_timestamps", "1", "temp_h_%03d.mp4", "-y");
        splitPb.inheritIO().start().waitFor();

        List<Path> frags = Files.list(Paths.get("."))
            .filter(p -> p.getFileName().toString().startsWith("temp_h_") && p.getFileName().toString().endsWith(".mp4"))
            .sorted().collect(Collectors.toList());

        long totalStart = System.currentTimeMillis();
        for (int i = 0; i < frags.size(); i++) {
            String out = cfg.outputFolder + "/inj_" + (i + 1) + ".mp4";
            System.out.print("\r" + C + "[INJECTING " + (i + 1) + "/" + frags.size() + "]" + RESET);
            
            if (i > 0) {
                double elapsed = (System.currentTimeMillis() - totalStart) / 1000.0;
                double remaining = (elapsed / i) * (frags.size() - i);
                System.out.print(Y + " | ETA: " + (int)remaining + "s    " + RESET);
            }

            String vFilter = "[0:v]scale=-2:400,pad=720:480:(720-iw)/2:40,setsar=1[m];" +
                             "[1:v]scale=720:40[t];[2:v]scale=720:40[b];" +
                             "[3:v]scale=100:-1,format=rgba,colorchannelmixer=aa=0.8[l];" +
                             "[m][t]overlay=0:0:shortest=1[v1];[v1][b]overlay=0:H-h:shortest=1[v2];[v2][l]overlay=20:50";

            ProcessBuilder renderPb = new ProcessBuilder(ffmpegPath, "-v", "error", "-i", frags.get(i).toString(),
                "-ignore_loop", "0", "-i", cfg.gifPath, "-ignore_loop", "0", "-i", cfg.gifPath,
                "-i", cfg.iconPath, "-filter_complex", vFilter, "-map", "0:a?", "-c:v", "libx264",
                "-b:v", "600k", "-preset", "superfast", "-shortest", out, "-y");

            renderPb.start().waitFor();
            Files.delete(frags.get(i));
        }

        System.out.println("\n\n" + G + BOLD + "[SUCCESS] BUG PAYLOAD INJECTED!" + RESET);
        if (cfg.deleteSource) videoFile.delete();
    }

    public static void main(String[] args) {
        Config cfg = new Config();
        if (args.length < 4) {
            System.out.println(BOLD + G + "Vinjector Java Edition v2.6" + RESET);
            System.out.println("Usage: java -jar Vinjector.jar -p <vid> -f <gif> -i <png> -d <sec> [-m <mb>] [-x]");
            return;
        }
        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "-p": cfg.videoPath = args[++i]; break;
                case "-f": cfg.gifPath = args[++i]; break;
                case "-i": cfg.iconPath = args[++i]; break;
                case "-d": cfg.duration = Integer.parseInt(args[++i]); break;
                case "-m": cfg.minSizeMb = Double.parseDouble(args[++i]); break;
                case "-x": cfg.deleteSource = true; break;
            }
        }
        try {
            Vinjector app = new Vinjector(cfg);
            app.showBugBanner();
            app.execute();
        } catch (Exception e) { 
            System.err.println(R + "Fatal Error: " + e.getMessage() + RESET);
        }
    }
}
