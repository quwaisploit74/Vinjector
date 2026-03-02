#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <getopt.h>
#include <algorithm>
#include <cstdio>
#include <memory>

namespace fs = std::filesystem;

// Hacker Palette
const std::string R = "\033[31m";    const std::string G = "\033[32m";
const std::string Y = "\033[33m";    const std::string B = "\033[34m";
const std::string M = "\033[35m";    const std::string C = "\033[36m";
const std::string W = "\033[37m";    const std::string BOLD = "\033[1m";
const std::string RESET = "\033[0m"; const std::string BLINK = "\033[5m";

struct Config {
    std::string gif_path, icon_path, video_path;
    int duration = 0;
    std::string output_folder = "video";
    double min_size_mb = 0.0;
    bool delete_source = false;
};

class Vinjector {
private:
    Config cfg;
    std::chrono::steady_clock::time_point start_time;

    void log(std::string tag, std::string msg, std::string color) {
        std::cout << W << "[" << color << BOLD << tag << RESET << W << "] " << msg << RESET << std::endl;
    }

    // Fungsi untuk mendapatkan durasi video menggunakan ffprobe
    double get_video_duration(std::string path) {
        std::string cmd = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 '" + path + "'";
        char buffer[128];
        std::string result = "";
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe) return 0;
        while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) result += buffer;
        try { return std::stod(result); } catch (...) { return 0; }
    }

    std::string format_eta(double seconds) {
        int h = (int)seconds / 3600;
        int m = ((int)seconds % 3600) / 60;
        int s = (int)seconds % 60;
        std::stringstream ss;
        if (h > 0) ss << h << "h ";
        if (m > 0 || h > 0) ss << m << "m ";
        ss << s << "s";
        return ss.str();
    }

public:
    Vinjector(Config c) : cfg(c) {}

    void show_snow_cow() {
        std::cout << C << "  * .   * .   * .   * .   * .   * .   * ." << RESET << std::endl;
        std::cout << W << "    * .  " << G << "  _______________________ " << W << " .   * . " << RESET << std::endl;
        std::cout << W << "  .   * " << G << "< " << W << BOLD << "VINJECTOR " << R << "v2.5-TEST" << G << " >" << W << "  * .   " << RESET << std::endl;
        std::cout << W << "    * .   " << G << "  ----------------------- " << W << ".   * .  " << RESET << std::endl;
        std::cout << W << "  * .     \\   ^__^" << RESET << std::endl;
        std::cout << W << "    .    * \\  (oo)\\_______" << RESET << std::endl;
        std::cout << W << "  * .   * (__)\\       )\\/\\" << RESET << std::endl;
        std::cout << W << "    .   * .        ||----w |" << RESET << std::endl;
        std::cout << W << "  * .   * .      ||     ||" << RESET << std::endl;
        std::cout << C << "  .   * .   * .   * .   * .   * .   * .   *" << RESET << std::endl;
        std::cout << M << "  AUTHOR:   " << W << "Muhammad Quwais Saputra" << RESET << std::endl;
        std::cout << G << "  STATUS:   " << BLINK << "CALCULATING VECTORS..." << RESET << "\n" << std::endl;
    }

    void execute() {
        if (!fs::exists(cfg.video_path)) {
            log("FAIL", "Target missing!", R);
            return;
        }

        // --- Predeteksi Estimasi ---
        double total_dur = get_video_duration(cfg.video_path);
        int est_frags = (total_dur > 0) ? std::ceil(total_dur / cfg.duration) : 0;
        
        log("SCAN", "Total duration: " + std::to_string((int)total_dur) + "s", C);
        log("SCAN", "Estimated sectors: " + std::to_string(est_frags), C);
        log("SCAN", "Analyzing hardware speed...", Y);
        std::cout << G << std::string(55, '=') << RESET << std::endl;

        if (!fs::exists(cfg.output_folder)) fs::create_directories(cfg.output_folder);

        // 1. Splitting
        std::string split_cmd = "ffmpeg -v error -i '" + cfg.video_path + "' -f segment -segment_time " + 
                                std::to_string(cfg.duration) + " -c copy -reset_timestamps 1 temp_h_%03d.mp4 -y";
        system(split_cmd.c_str());

        std::vector<std::string> frags;
        for (const auto& entry : fs::directory_iterator(".")) {
            std::string name = entry.path().filename().string();
            if (name.find("temp_h_") == 0 && name.find(".mp4") != std::string::npos) frags.push_back(name);
        }
        std::sort(frags.begin(), frags.end());

        // 2. Processing with ETA
        auto total_start = std::chrono::steady_clock::now();
        
        for (size_t i = 0; i < frags.size(); ++i) {
            auto frag_start = std::chrono::steady_clock::now();
            std::string out = cfg.output_folder + "/inj_" + std::to_string(i + 1) + ".mp4";
            
            // Progress Info
            std::cout << "\r" << C << "[PROCESSING " << (i+1) << "/" << frags.size() << "]" << RESET;
            if (i > 0) {
                auto now = std::chrono::steady_clock::now();
                double elapsed = std::chrono::duration<double>(now - total_start).count();
                double avg_time_per_frag = elapsed / i;
                double remaining = avg_time_per_frag * (frags.size() - i);
                std::cout << Y << " | ETA: " << format_eta(remaining) << "    " << RESET;
            }
            std::cout << std::flush;

            std::string v_filter = "\"[0:v]scale=-2:400,pad=720:480:(720-iw)/2:40,setsar=1[m];[1:v]scale=720:40[t];[2:v]scale=720:40[b];[3:v]scale=100:-1,format=rgba,colorchannelmixer=aa=0.8[l];[m][t]overlay=0:0:shortest=1[v1];[v1][b]overlay=0:H-h:shortest=1[v2];[v2][l]overlay=20:50\"";
            
            std::string cmd = "ffmpeg -v error -i '" + frags[i] + "' -ignore_loop 0 -i '" + cfg.gif_path + 
                              "' -ignore_loop 0 -i '" + cfg.gif_path + "' -i '" + cfg.icon_path + 
                              "' -filter_complex " + v_filter + " -map 0:a? -c:v libx264 -b:v 600k -preset superfast -shortest '" + out + "' -y";
            system(cmd.c_str());

            if (fs::exists(out) && fs::file_size(out) / (1024.0 * 1024.0) < cfg.min_size_mb) fs::remove(out);
            fs::remove(frags[i]);
        }

        auto total_end = std::chrono::steady_clock::now();
        double total_elapsed = std::chrono::duration<double>(total_end - total_start).count();

        std::cout << "\n\n" << G << BOLD << "[SUCCESS] ALL SECTORS INJECTED!" << RESET << std::endl;
        log("TIME", "Total process time: " + format_eta(total_elapsed), G);

        if (cfg.delete_source) fs::remove(cfg.video_path);
    }
};

int main(int argc, char* argv[]) {
    Config cfg; int opt;
    while ((opt = getopt(argc, argv, "f:i:p:d:n:m:x")) != -1) {
        switch (opt) {
            case 'f': cfg.gif_path = optarg; break;
            case 'i': cfg.icon_path = optarg; break;
            case 'p': cfg.video_path = optarg; break;
            case 'd': cfg.duration = std::stoi(optarg); break;
            case 'n': cfg.output_folder = optarg; break;
            case 'm': cfg.min_size_mb = std::stod(optarg); break;
            case 'x': cfg.delete_source = true; break;
        }
    }
    if (cfg.gif_path.empty() || cfg.video_path.empty()) return 1;
    system("clear");
    Vinjector app(cfg);
    app.show_snow_cow();
    app.execute();
    return 0;
}
