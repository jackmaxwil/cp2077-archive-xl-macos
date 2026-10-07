#include "SpdlogProvider.hpp"
#include "Core/Facades/Runtime.hpp"
#include "Core/Stl.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <fstream>
#include <iostream>

void Support::SpdlogProvider::OnInitialize()
{
    if (m_baseLogPath.empty())
    {
        m_baseLogPath = Core::Runtime::GetModulePath().replace_extension(L".log");
    }

    auto logPath = m_baseLogPath;

    if (m_appendTimestamp)
    {
        const auto logExtension = m_baseLogPath.extension();

        logPath.replace_extension();
        logPath += "-";

        if (m_maxLogCount > 0)
        {
            std::error_code error;
            std::set<std::filesystem::path> existingLogs;

            for (const auto& entry : std::filesystem::directory_iterator(m_baseLogPath.parent_path(), error))
            {
                if (entry.is_regular_file() && entry.path().extension() == logExtension &&
                    entry.path().wstring().starts_with(logPath.wstring()))
                {
                    existingLogs.insert(entry.path());
                }
            }

            auto excessiveLogCount = static_cast<int32_t>(existingLogs.size()) - m_maxLogCount + 1;
            if (excessiveLogCount > 0)
            {
                for (const auto& path : existingLogs)
                {
                    std::filesystem::remove(path, error);

                    if (--excessiveLogCount == 0)
                    {
                        break;
                    }
                }
            }
        }

        // Append timestamp to filename
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_c);

        const auto logTimestamp =
            fmt::format("{:04d}-{:02d}-{:02d}-{:02d}-{:02d}-{:02d}", now_tm.tm_year + 1900, now_tm.tm_mon + 1,
                        now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);

        logPath += logTimestamp;
        logPath.replace_extension(logExtension);
    }

    m_resolvedLogPath = logPath;

#if defined(_WIN32) || defined(_WIN64)
    auto sink = Core::MakeShared<spdlog::sinks::rotating_file_sink_mt>(logPath.string(), m_maxPartSize, m_maxPartCount);
    auto logger = Core::MakeShared<spdlog::logger>("", spdlog::sinks_init_list{sink});
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %v");
    logger->flush_on(spdlog::level::trace);

    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::trace);
#else
    // macOS: avoid spdlog runtime issues during early injection; write to file manually.
    // Ensure the log file exists early so users can tail it even if later steps crash.
    std::error_code ec;
    std::filesystem::create_directories(m_resolvedLogPath.parent_path(), ec);
    std::ofstream f(m_resolvedLogPath, std::ios::app);
    (void)f;
#endif

    if (m_recentSymlink && logPath != m_baseLogPath)
    {
        std::error_code error;
        std::filesystem::remove(m_baseLogPath, error);
        std::filesystem::create_symlink(logPath.filename(), m_baseLogPath, error);
    }

    SetDefault(*this);
}

void Support::SpdlogProvider::LogInfo(const std::string_view& aMessage)
{
#if !defined(_WIN32) && !defined(_WIN64)
    {
        std::scoped_lock _{m_logMutex};
        if (!m_resolvedLogPath.empty())
        {
            std::ofstream f(m_resolvedLogPath, std::ios::app);
            f << "[INFO] " << aMessage << "\n";
        }
    }
    std::cerr << "[ArchiveXL INFO] " << aMessage << std::endl;
#else
    spdlog::default_logger_raw()->info(aMessage);
#endif
}

void Support::SpdlogProvider::LogWarning(const std::string_view& aMessage)
{
#if !defined(_WIN32) && !defined(_WIN64)
    {
        std::scoped_lock _{m_logMutex};
        if (!m_resolvedLogPath.empty())
        {
            std::ofstream f(m_resolvedLogPath, std::ios::app);
            f << "[WARN] " << aMessage << "\n";
        }
    }
    std::cerr << "[ArchiveXL WARN] " << aMessage << std::endl;
#else
    spdlog::default_logger_raw()->warn(aMessage);
#endif
}

void Support::SpdlogProvider::LogError(const std::string_view& aMessage)
{
#if !defined(_WIN32) && !defined(_WIN64)
    {
        std::scoped_lock _{m_logMutex};
        if (!m_resolvedLogPath.empty())
        {
            std::ofstream f(m_resolvedLogPath, std::ios::app);
            f << "[ERROR] " << aMessage << "\n";
        }
    }
    std::cerr << "[ArchiveXL ERROR] " << aMessage << std::endl;
#else
    spdlog::default_logger_raw()->error(aMessage);
#endif
}

void Support::SpdlogProvider::LogDebug(const std::string_view& aMessage)
{
#if !defined(_WIN32) && !defined(_WIN64)
    {
        std::scoped_lock _{m_logMutex};
        if (!m_resolvedLogPath.empty())
        {
            std::ofstream f(m_resolvedLogPath, std::ios::app);
            f << "[DEBUG] " << aMessage << "\n";
        }
    }
    std::cerr << "[ArchiveXL DEBUG] " << aMessage << std::endl;
#else
    spdlog::default_logger_raw()->debug(aMessage);
#endif
}

void Support::SpdlogProvider::LogFlush()
{
#if defined(_WIN32) || defined(_WIN64)
    spdlog::default_logger_raw()->flush();
#endif
}
