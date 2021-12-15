#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <type_traits>
#include <vector>

#include "AsstUtils.hpp"
#include "Version.h"

namespace asst
{
    class Logger
    {
    public:
        ~Logger() = default;

        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;

        static Logger& get_instance()
        {
            static Logger _unique_instance;
            return _unique_instance;
        }

        static void set_dirname(std::string dirname) noexcept
        {
            m_dirname = std::move(dirname);
        }

        template <typename... Args>
        inline void trace(Args&&... args)
        {
            std::string_view level = "TRC";
            log(level, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void info(Args&&... args)
        {
            std::string_view level = "INF";
            log(level, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void error(Args&&... args)
        {
            std::string_view level = "ERR";
            log(level, std::forward<Args>(args)...);
        }

        const std::string m_log_filename = m_dirname + "asst.log";
        const std::string m_log_bak_filename = m_dirname + "asst.bak.log";

    private:
        Logger()
        {
            check_filesize_and_remove();
            log_init_info();
        }

        void check_filesize_and_remove()
        {
            constexpr uintmax_t MaxLogSize = 4 * 1024 * 1024;
            try {
                if (std::filesystem::exists(m_log_filename)) {
                    uintmax_t log_size = std::filesystem::file_size(m_log_filename);
                    if (log_size >= MaxLogSize) {
                        std::filesystem::rename(m_log_filename, m_log_bak_filename);
                    }
                }
            }
            catch (...) {
                ;
            }
        }
        void log_init_info()
        {
            trace("-----------------------------");
            trace("MeoAssistant Process Start");
            trace("Version", asst::Version);
            trace("Build DataTime", __DATE__, __TIME__);
            trace("Working Path", m_dirname);
            trace("-----------------------------");
        }

        template <typename... Args>
        void log(std::string_view level, Args&&... args)
        {
            std::unique_lock<std::mutex> trace_lock(m_trace_mutex);

            char buff[128] = { 0 };
            sprintf_s(buff, "[%s][%s][Px%x][Tx%x]",
                      asst::utils::get_format_time().c_str(),
                      level.data(), _getpid(), ::GetCurrentThreadId());

            std::ofstream ofs(m_log_filename, std::ios::out | std::ios::app);
#ifdef LOG_TRACE
            stream_args(ofs, buff, args...);
#else
            stream_args(ofs, buff, std::forward<Args>(args)...);
#endif
            ofs.close();

#ifdef LOG_TRACE
            stream_args<true>(std::cout, buff, std::forward<Args>(args)...);
#endif
        }

        template <bool ToGbk = false, typename T, typename... Args>
        inline void stream_args(std::ostream& os, T&& first, Args&&... rest)
        {
            stream<ToGbk, T>()(os, std::forward<T>(first));
            stream_args<ToGbk>(os, std::forward<Args>(rest)...);
        }
        template <bool>
        inline void stream_args(std::ostream& os)
        {
            os << std::endl;
        }

        template <bool ToGbk, typename T, typename = void>
        struct stream
        {
            inline void operator()(std::ostream& os, T&& first)
            {
                os << first << " ";
            }
        };
        template <typename T>
        struct stream<true, T, typename std::enable_if<std::is_constructible<std::string, T>::value>::type>
        {
            inline void operator()(std::ostream& os, T&& first)
            {
                os << utils::utf8_to_gbk(first) << " ";
            }
        };

        inline static std::string m_dirname;
        std::mutex m_trace_mutex;
    };

    class LoggerAux
    {
    public:
        LoggerAux(const std::string& func_name)
            : m_func_name(func_name),
            m_start_time(std::chrono::steady_clock::now())
        {
            Logger::get_instance().trace(m_func_name, " | enter");
        }
        ~LoggerAux()
        {
            auto duration = std::chrono::steady_clock::now() - m_start_time;
            Logger::get_instance().trace(m_func_name, " | leave,",
                                         std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), "ms");
        }

    private:
        std::string m_func_name;
        std::chrono::time_point<std::chrono::steady_clock> m_start_time;
    };

    //static auto& log = Logger::get_instance();
#define Log Logger::get_instance()
#define LogTraceFunction LoggerAux _func_aux(__FUNCTION__)
#define LogTraceScope LoggerAux _func_aux
}
