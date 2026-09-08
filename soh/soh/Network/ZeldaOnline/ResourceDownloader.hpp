#ifndef RESOURCEDOWNLOADERH
#define RESOURCEDOWNLOADERH

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ZeldaOnline {

enum DownloadResourceType {
    DOWNLOAD_RESOURCE_TYPE_PLAYER_SKIN,
};

struct DownloadedFile {
    std::string fileName;
    std::string filePath;
    bool success;
    std::string error;
};

struct DownloadResult {
    DownloadResourceType type;
    std::string resourceName;
    std::vector<DownloadedFile> files;

    bool AllSucceeded() const {
        for (const DownloadedFile& file : files) {
            if (!file.success) {
                return false;
            }
        }
        return !files.empty();
    }
};

class ResourceDownloader {
  public:
    ~ResourceDownloader();

    bool Request(const std::vector<std::string>& urls, const std::vector<std::string>& fileNames,
                 const std::string& resourceName, DownloadResourceType type);

    std::vector<DownloadResult> TakeCompleted();

    void CancelAll();
    void JoinAll();

    static bool IsSafeFileName(const std::string& name);

  private:
    static void Worker(ResourceDownloader* self, std::vector<std::string> urls, std::vector<std::string> fileNames,
                       std::string resourceName, DownloadResourceType type);
    static DownloadedFile Fetch(ResourceDownloader* self, const std::string& url, const std::string& fileName);

    void Complete(const DownloadResult& result);

    std::atomic<bool> m_cancel{ false };

    std::mutex m_mutex;
    std::vector<DownloadResult> m_completed;
    std::vector<std::thread> m_threads;
};

} // namespace ZeldaOnline

#endif