#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sys/stat.h>

namespace fs = std::filesystem;

class Vinjector {
private:
    struct Config {
        std::string program = "Vinjector";
        std::string version = "1.0-BETA";
        std::string codename = "INJECTORIOR";
        std::string author = "Muhammad Quwais Saputra";
        std::string download_dir = "/data/data/com.termux/files/home/storage/downloads";
        float min_size_mb = 8.0;
        int max_folders = 10;
        int files_per_dir = 5;
        std::string gif_path = std::string(getenv("HOME")) + "/a.gif";
        std::string png_path = std::string(getenv("HOME")) + "/b.png";
        int duration = 120;
    } config;

    void log_status(char symbol, std::string msg) {
        std::cout << "[" << symbol << "] " << msg << std::endl;
    }

    void spinning_loader(std::string label, int current, int total) {
        const char chars[] = {'|', '/', '-', '\\'};
        for (int i = 0; i < 5; ++i) {
            std::cout << "\r[" << label << "] " << chars[i % 4] 
                      << " Memproses bagian " << current << "/" << total << "..." << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::pair<std::string, int> find_next_slot() {
        for (int i = 1; i <= config.max_folders; ++i) {
            std::string folder = "edit_" + std::to_string(i);
            if (!fs::exists(folder)) fs::create_directory(folder);

            for (int j = 1; j <= config.files_per_dir; ++j) {
                std::string path = folder + "/video_" + std::to_string(j) + ".mp4";
                if (!fs::exists(path)) return {folder, j};
            }
        }
        return {"", 0};
    }

    std::string escape_shell(std::string s) {
        return "'" + s + "'"; // Sederhana untuk handle spasi di Linux
    }

public:
    void print_banner() {
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "Starting " << config.program << " " << config.version << std::endl;
        std::cout << "Codename: " << config.codename << std::endl;
        std::cout << "Author  : " << config.author << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
    }

    void run() {
        if (!fs::exists(config.download_dir)) {
            log_status('!', "Direktori tidak ditemukan, membuat folder...");
            fs::create_directories(config.download_dir);
        }
        fs::current_path(config.download_dir);

        std::vector<fs::path> targets;
        for (const auto& entry : fs::directory_iterator(".")) {
            std::string filename = entry.path().filename().string();
            if (entry.path().extension() == ".mp4" && filename.find("temp_h_") != 0) {
                targets.push_back(entry.path());
            }
        }

        if (targets.empty()) {
            log_status('X', "Tidak ada aset MP4!");
            return;
        }

        // Urutkan berdasarkan waktu modifikasi (terbaru)
        std::sort(targets.begin(), targets.end(), [](const fs::path& a, const fs::path& b) {
            return fs::last_write_time(a) > fs::last_write_time(b);
        });

        std::cout << "\nPILIH TARGET ASSET:" << std::endl;
        for (size_t i = 0; i < targets.size(); ++i) {
            double size = static_cast<double>(fs::file_size(targets[i])) / (1024 * 1024);
            std::cout << "  (" << i + 1 << ") " << std::left << std::setw(30) 
                      << targets[i].filename().string() << " [" << std::fixed << std::setprecision(2) << size << " MB]" << std::endl;
        }

        std::cout << "\n>> Masukkan ID: ";
        int choice;
        std::cin >> choice;

        if (choice > 0 && choice <= targets.size()) {
            std::string target = targets[choice - 1].filename().string();
            log_status('!', "Memotong video menjadi fragmen " + std::to_string(config.duration) + " detik...");

            std::string cmd_split = "ffmpeg -v error -i " + escape_shell(target) + 
                                    " -f segment -segment_time " + std::to_string(config.duration) + 
                                    " -c copy -reset_timestamps 1 temp_h_%03d.mp4 -y";
            system(cmd_split.c_str());

            std::vector<std::string> fragments;
            for (const auto& entry : fs::directory_iterator(".")) {
                if (entry.path().filename().string().find("temp_h_") == 0) {
                    fragments.push_back(entry.path().filename().string());
                }
            }
            std::sort(fragments.begin(), fragments.end());

            if (!fragments.empty()) {
                log_status('*', "Berhasil memotong menjadi " + std::to_string(fragments.size()) + " fragmen.");
                std::cout << std::string(50, '-') << std::endl;

                for (size_t i = 0; i < fragments.size(); ++i) {
                    auto [folder, idx] = find_next_slot();
                    if (folder == "") break;

                    std::string output = folder + "/video_" + std::to_string(idx) + ".mp4";
                    std::string v_filter = "\"[0:v]scale=-2:400,pad=720:480:(720-iw)/2:40,setsar=1[m];"
                                           "[1:v]scale=720:40[t];[2:v]scale=720:40[b];"
                                           "[3:v]scale=100:-1,format=rgba,colorchannelmixer=aa=0.8[l];"
                                           "[m][t]overlay=0:0:shortest=1[v1];[v1][b]overlay=0:H-h:shortest=1[v2];"
                                           "[v2][l]overlay=20:50\"";

                    spinning_loader("RENDERING", i + 1, fragments.size());

                    std::string cmd_render = "ffmpeg -v error -i " + escape_shell(fragments[i]) + 
                                             " -ignore_loop 0 -i " + escape_shell(config.gif_path) + 
                                             " -ignore_loop 0 -i " + escape_shell(config.gif_path) + 
                                             " -i " + escape_shell(config.png_path) + 
                                             " -filter_complex " + v_filter + 
                                             " -map 0:a? -c:v libx264 -b:v 550k -preset superfast -c:a aac -b:a 64k -shortest " + 
                                             escape_shell(output) + " -y";
                    
                    system(cmd_render.c_str());

                    // Cek ukuran minimal
                    if (fs::exists(output) && (fs::file_size(output) / (1024.0 * 1024.0)) < config.min_size_mb) {
                        fs::remove(output);
                    }
                    fs::remove(fragments[i]);
                }

                std::cout << "\r[*] PROSES SELESAI 100%                                \n";
                fs::remove(target);
                log_status('*', "Urutan injeksi selesai.");
            }
        } else {
            log_status('X', "ID tidak valid.");
        }
    }
};

int main() {
    Vinjector app;
    app.print_banner();
    app.run();
    std::cout << "Semua pekerjaan beres." << std::endl;
    return 0;
}
