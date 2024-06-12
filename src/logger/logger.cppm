export module logger;

import <array>;
import <string>;
import <print>;
import <format>;
import <string_view>;
import <cstring>;
import <cstddef>;
import <cstdlib>;
import <exception>;

class _logger_manager
{
	void* ptr = nullptr;

public:
	_logger_manager() = default;
	_logger_manager(const _logger_manager&) = delete;
	_logger_manager(_logger_manager&&) = delete;
	~_logger_manager()
	{
		if (ptr)
			free(ptr);
	}

	_logger_manager& operator=(const _logger_manager&) = delete;
	_logger_manager& operator=(_logger_manager&&) = delete;

	void reset(void*&& ptr)
	{
		if (this->ptr)
			free(this->ptr);
		this->ptr = ptr;
		ptr = nullptr;
	}

	auto get_raw() const
	{
		return ptr;
	}

	template<class T>
	T* get() const
	{
		if (!ptr)
			std::runtime_error(std::format("Tried to _logger_manager::get() as '{}' when it's uninitialized", typeid(T).name()));
		return static_cast<T*>(ptr);
	}
} _global_logger;

export class logger
{
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
	std::string m_name = "unassigned";

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
		if (static_cast<int>(lvl) == -1)
		{
			auto current_logger = _global_logger.get<logger>();
			m_level = current_logger->m_level;
		} else m_level = default_level;
		m_name = name;
	}

	void level(level lvl)
	{
		m_level = lvl;
	}
	auto level() const
	{
		return m_level;
	}

	static void init(enum level lvl = default_level)
	{
		if (!_global_logger.get_raw())
		{
			auto ptr_raw = malloc(sizeof(logger));
			if (!ptr_raw)
				exception::enact("Failed to allocate memory for the global logger");
			_global_logger.reset(std::move(ptr_raw));
			_global_logger.get<logger>()->m_level = lvl;
			_global_logger.get<logger>()->m_name = "global";
		}
	}

	static logger* global()
	{
		return _global_logger.get<logger>();
	}

	template<class... Args>
	void log(enum level lvl, const std::string_view format, Args&&... args)
	{
		auto current_logger = _global_logger.get<logger>();
		if (static_cast<int>(lvl) >= static_cast<int>(current_logger->m_level) && lvl != level::off)
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
