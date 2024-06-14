export module logger;

import <array>;
import <string>;
import <print>;
import <format>;
import <string_view>;
import <cstddef>;
import <cstring>;
import <cstdlib>;
import <exception>;

class logger_manager
{
	void* ptr = nullptr;

public:
	logger_manager() = default;
	logger_manager(const logger_manager&) = delete;
	logger_manager(logger_manager&&) = delete;
	logger_manager& operator=(const logger_manager&) = delete;
	logger_manager& operator=(logger_manager&&) = delete;

	void set(void*&& ptr)
	{
		if (this->ptr)
			throw std::runtime_error("logger_manager can only be ::set() once");

		this->ptr = ptr;
		ptr = nullptr;
	}

	void unset()
	{
		this->ptr = nullptr;
	}

	auto get_raw() const
	{
		return ptr;
	}

	template<class T>
	T* get() const
	{
		return static_cast<T*>(ptr);
	}

	void ensure_initialized()
	{
		if (!ptr)
			throw std::runtime_error(std::format("Initialize the global logger before attemping to use any of it's functions"));
	}
} global_logger_manager;

export class logger
{
	static inline size_t ref_count = 0;

public:
	enum class level
	{
		debug,
		info,
		warn,
		error,
		criticial,
		off
	} m_level;
	std::string m_name;

	static constexpr std::array<std::string, 6> level_str {
		"debug",
		"info",
		"warn",
		"error",
		"critical",
		"off"
	};
	static constexpr level default_level = level::info;

public:
	logger(std::string_view name, level lvl = static_cast<enum level>(-1))
	{
		global_logger_manager.ensure_initialized();

		if (static_cast<int>(lvl) == -1)
			m_level = global()->m_level;
		else
			m_level = default_level;
		m_name = name;

		ref_count++;
	}

	~logger()
	{
		ref_count--;

		if (ref_count == 0)
			global_destroy();
	}

	void level(level lvl)
	{
		m_level = lvl;
	}
	auto level() const
	{
		return m_level;
	}

	static void global_init(enum level lvl = default_level)
	{
		if (!global_logger_manager.get_raw())
		{
			auto global_logger = malloc(sizeof(logger));
			if (!global_logger)
				throw std::runtime_error("Failed to allocate memory for the global logger");

			memset(global_logger, 0, sizeof(logger));

			global_logger_manager.set(std::move(global_logger));

			global()->m_name = "global";
			global()->m_level = lvl;
		}
	}

	static logger* global()
	{
		return global_logger_manager.get<logger>();
	}

	template<class... Args>
	void log(enum level lvl, const std::string_view format, Args&&... args)
	{
		if (static_cast<int>(lvl) >= static_cast<int>(global()->m_level) && lvl != level::off)
			std::vprint_unicode(
				stderr,
				std::format("[{}] {}: {}\n", level_str[static_cast<int>(lvl)], m_name, format),
				std::make_format_args(args...));
	}

	template<class... Args>
	void debug(std::string_view format, Args&&... args)
	{
		log(level::debug, format, args...);
	}

	template<class... Args>
	void info(std::string_view format, Args&&... args)
	{
		log(level::info, format, args...);
	}

	template<class... Args>
	void warn(std::string_view format, Args&&... args)
	{
		log(level::warn, format, args...);
	}

	template<class... Args>
	void error(std::string_view format, Args&&... args)
	{
		log(level::error, format, args...);
	}

	template<class... Args>
	void critical(std::string_view format, Args&&... args)
	{
		log(level::criticial, format, args...);
	}

private:
	static void global_destroy()
	{
		global()->~logger();

		free(global_logger_manager.get_raw());
		global_logger_manager.unset();
	}

public:
	class exception : public std::runtime_error
	{
	public:
		exception(std::string_view msg)
			:std::runtime_error(msg.data())
		{}

		template<class... Args>
		static void enact(std::string_view format, Args&&... args)
		{
			throw exception(std::vformat(format, std::make_format_args(args...)));
		}
	};
};
