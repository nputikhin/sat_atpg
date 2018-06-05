#pragma once

#include <ostream>
#include <type_traits>

#define ENABLE_LOGGING 1
#define USE_ANSI_ESCAPE_CODES 0

#if(ENABLE_LOGGING)
#define log_info() Logger(Logger::LogLevel::Info)
#define log_warning() Logger(Logger::LogLevel::Warning, "Warning")
#define log_error() Logger(Logger::LogLevel::Error, "Error")
#define log_debug() Logger(Logger::LogLevel::Debug, "DEBUG", __FILE__, __LINE__)
#define log_noendl Logger::Noendl()
#define log_nospace Logger::Nospace()
#else
#define log_info() NoLogger()
#define log_warning() NoLogger()
#define log_error() NoLogger()
#define log_debug() NoLogger()
#define log_noendl 0
#define log_nospace 0

class NoLogger
{
public:
	template<typename T>
	NoLogger& operator<<(const T& val) { return *this; }
};
#endif

enum LogColor
{
	fgDefault = 39,
	bgDefaut = 49,
	fgBlack = 30,
	bgBlack = 40,
	fgRed = 31,
	bgRed = 41,
	fgGreen = 32,
	bgGreen = 42,
	fgYellow = 33,
	bgYellow = 43,
	fgBlue = 34,
	bgBlue = 44,
	fgMagenta = 35,
	bgMagenta = 45,
	fgCyan = 36,
	bgCyan = 46,
	fgWhite = 37,
	bgWhite = 47,
	fgBrightBlack = 90,
	bgBrightBlack = 100,
	fgBrightRed = 91,
	bgBrightRed = 101,
	fgBrightGreen = 92,
	bgBrightGreen = 102,
	fgBrightYellow = 93,
	bgBrightYellow = 103,
	fgBrightBlue = 94,
	bgBrightBlue = 104,
	fgBrightMagenta = 95,
	bgBrightMagenta = 105,
	fgBrightCyan = 96,
	bgBrightCyan = 106,
	fgBrightWhite = 97,
	bgBrightWhite = 107,
};

class Logger
{
public:
	enum LogLevel
	{
		None,
		Error,
		Warning,
		Info,
		Debug
	};


	struct Noendl {};
	struct Nospace {};

	Logger(LogLevel level, const char* prefix = nullptr, const char* file = nullptr, int line = 0)
	{
		m_level = level;
		if (m_level <= s_log_level) {
			if (m_level == LogLevel::Error) {
				*this << (LogColor::fgBrightRed);
			} else if (m_level == LogLevel::Warning) {
				*this << (LogColor::fgBrightYellow);
			}
			if (prefix)
				s_log_stream << prefix;
			if (file)
				s_log_stream << '(' << file;
			if (line) {
				s_log_stream << ':';
				s_log_stream << line;
			}
			if (file)
				s_log_stream << ')';
			if (prefix)
				s_log_stream << ": ";
		}
	}

	~Logger()
	{
		if (m_level <= s_log_level) {
			if (m_need_reset_font) {
				*this << LogColor::fgDefault << LogColor::bgDefaut;
			}
			if (m_endl)
				s_log_stream << std::endl;
			else
				s_log_stream.flush();
		}
	}

	static void set_log_level(LogLevel level) { s_log_level = level; }
	static void set_ostream(std::streambuf* buf) { s_log_stream.rdbuf(buf); }

	template<typename T>
	Logger& operator<<(const T& val)
	{
		write<T>(val);
		return *this;
	}

	Logger& operator<<(const LogColor color_code)
	{
		(void)color_code;
		m_need_reset_font = true;
#if USE_ANSI_ESCAPE_CODES
		s_log_stream << std::string("\033[") + std::to_string(color_code) + "m";
#endif
		return *this;
	}

	Logger& operator<<(const Noendl&)
	{
		m_endl = false;
		return *this;
	}

	Logger& operator<<(const Nospace&)
	{
		m_space = false;
		return *this;
	}

private:
	template <typename T>
	struct has_begin_end
	{
	private:
		template<typename C>
		static
		typename std::enable_if<
				(
					decltype(std::declval<C>().cbegin(), std::true_type())::value
					&& decltype(std::declval<C>().cend(), std::true_type())::value
				)
			, std::true_type>::type
		test(int);

		template<typename C>
		static std::false_type test(...);

	public:
		static constexpr bool value = std::is_same<decltype(test<T>(0)), std::true_type>::value;
	};


	template<typename T>
	struct TypeSelector
	{
		static constexpr bool is_iterable = has_begin_end<T>::value && !std::is_convertible<T, std::string>::value;
		static constexpr bool is_enum = std::is_enum<T>::value;
		static constexpr bool is_basic = !is_iterable && !is_enum;
	};


	template<typename T>
	auto write(const T& val) ->
	typename std::enable_if<TypeSelector<T>::is_iterable, void>::type
	{
		bool space = m_space;
		m_space = false;
		write("{ ");
		for (auto it = val.cbegin(); it != val.cend(); ++it) {
			if (it != val.cbegin()) {
				write(", ");
			}
			write(*it);
		}
		write(" }");
		m_space = space;
	}

	template<typename T>
	auto write(const T& val) ->
	typename std::enable_if<TypeSelector<T>::is_enum, void>::type
	{
		write(static_cast<typename std::underlying_type<T>::type>(val));
	}

	template<typename T>
	auto write(const T& val) ->
	typename std::enable_if<TypeSelector<T>::is_basic, void>::type
	{
		if (m_level <= s_log_level) {
			s_log_stream << val;
			if (m_space)
				s_log_stream << ' ';
		}
	}

	LogLevel m_level = LogLevel::Info;
	bool m_endl = true;
	bool m_space = true;

	bool m_need_reset_font = false;

	static LogLevel s_log_level;
	static std::ostream s_log_stream;
};

