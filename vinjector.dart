import 'dart:io';
import 'dart:async';
import 'dart:convert';

class Config {
  String? videoPath;
  String? gifPath;
  String? iconPath;
  int duration = 0;
  String outputFolder = "video";
  bool deleteSource = false;
}

class Vinjector {
  // Hacker Palette
  static const String R = "\x1B[31m";
  static const String G = "\x1B[32m";
  static const String Y = "\x1B[33m";
  static const String B = "\x1B[34m";
  static const String M = "\x1B[35m";
  static const String C = "\x1B[36m";
  static const String W = "\x1B[37m";
  static const String BOLD = "\x1B[1m";
  static const String RESET = "\x1B[0m";

  final Config cfg;
  final String ffmpegPath = "ffmpeg";
  final String ffprobePath = "ffprobe";

  Vinjector(this.cfg);

  void _log(String tag, String msg, String color) {
    print("$W[$color$BOLD$tag$RESET$W] $msg$RESET");
  }

  Future<double> _getVideoDuration(String path) async {
    try {
      final result = await Process.run(ffprobePath, [
        "-v", "error", "-show_entries", "format=duration",
        "-of", "default=noprint_wrappers=1:nokey=1", path
      ]);
      return double.tryParse(result.stdout.toString().trim()) ?? 0;
    } catch (e) {
      return 0;
    }
  }

  void showBugBanner() {
    final List<String> bugLines = [
      "$C  * .  * .  * .  * .  * .  * .  * .  * .  *",
      "$W          __      $G  ________________________",
      "$W        .d$$b     $G < $W${BOLD}VINJECTOR Dart-v2.6 $G>",
      "$W      .' TO\$;\\    $G  ------------------------",
      "$W     /  : TP._;   $C  * .  * .  * .  * .  *",
      "$M  TARGET: $W Termux / Dart Runtime",
      "$M  AUTHOR: $W M. Quwais Saputra",
      "$W  _.-'       d$$$$",
      "$W.' ..       d$$$$; $G$BOLD[ SYSTEM READY ]",
    ];

    for (var line in bugLines) {
      print(line);
      sleep(const Duration(milliseconds: 25));
    }
    print("");
  }

  Future<void> execute() async {
    final videoFile = File(cfg.videoPath!);
    if (!await videoFile.exists()) {
      _log("FAIL", "Target video missing!", R);
      return;
    }

    double totalDur = await _getVideoDuration(cfg.videoPath!);
    if (totalDur == 0) {
      _log("FAIL", "FFmpeg cannot read duration!", R);
      return;
    }

    int estFrags = (totalDur / cfg.duration).ceil();
    _log("SCAN", "Total duration: ${totalDur.toInt()}s", C);
    _log("SCAN", "Estimated sectors: $estFrags", C);
    print("$G${"=" * 55}$RESET");

    await Directory(cfg.outputFolder).create(recursive: true);

    _log("INFO", "Splitting source...", Y);
    await Process.run(ffmpegPath, [
      "-v", "error", "-i", cfg.videoPath!, "-f", "segment",
      "-segment_time", cfg.duration.toString(), "-c", "copy",
      "-reset_timestamps", "1", "temp_h_%03d.mp4", "-y"
    ]);

    final dir = Directory('.');
    final frags = await dir.list()
        .where((entity) => entity is File && entity.path.contains("temp_h_") && entity.path.endsWith(".mp4"))
        .cast<File>()
        .toList();
    frags.sort((a, b) => a.path.compareTo(b.path));

    final startTime = DateTime.now();

    for (int i = 0; i < frags.length; i++) {
      String out = "${cfg.outputFolder}/inj_${i + 1}.mp4";
      
      String etaStr = "";
      if (i > 0) {
        final elapsed = DateTime.now().difference(startTime).inSeconds;
        final remaining = ((elapsed / i) * (frags.length - i)).toInt();
        etaStr = " | ETA: ${remaining}s";
      }

      stdout.write("\r$C[INJECTING ${i + 1}/${frags.length}]$RESET$Y$etaStr    $RESET");

      const vFilter = "[0:v]scale=-2:400,pad=720:480:(720-iw)/2:40,setsar=1[m];"
                      "[1:v]scale=720:40[t];[2:v]scale=720:40[b];"
                      "[3:v]scale=100:-1,format=rgba,colorchannelmixer=aa=0.8[l];"
                      "[m][t]overlay=0:0:shortest=1[v1];[v1][b]overlay=0:H-h:shortest=1[v2];[v2][l]overlay=20:50";

      await Process.run(ffmpegPath, [
        "-v", "error", "-i", frags[i].path,
        "-ignore_loop", "0", "-i", cfg.gifPath!,
        "-ignore_loop", "0", "-i", cfg.gifPath!,
        "-i", cfg.iconPath!, "-filter_complex", vFilter,
        "-map", "0:a?", "-c:v", "libx264", "-b:v", "600k",
        "-preset", "superfast", "-shortest", out, "-y"
      ]);

      await frags[i].delete();
    }

    print("\n\n$G$BOLD[SUCCESS] PAYLOAD INJECTED!$RESET");
    if (cfg.deleteSource) await videoFile.delete();
  }
}

void main(List<String> args) async {
  final cfg = Config();
  
  if (args.length < 4) {
    print("${Vinjector.BOLD}${Vinjector.G}Vinjector Dart Edition$RESET");
    print("Usage: dart vinjector.dart -p <vid> -f <gif> -i <png> -d <sec> [-x]");
    return;
  }

  for (int i = 0; i < args.length; i++) {
    switch (args[i]) {
      case "-p": cfg.videoPath = args[++i]; break;
      case "-f": cfg.gifPath = args[++i]; break;
      case "-i": cfg.iconPath = args[++i]; break;
      case "-d": cfg.duration = int.parse(args[++i]); break;
      case "-x": cfg.deleteSource = true; break;
    }
  }

  final app = Vinjector(cfg);
  app.showBugBanner();
  await app.execute();
}
