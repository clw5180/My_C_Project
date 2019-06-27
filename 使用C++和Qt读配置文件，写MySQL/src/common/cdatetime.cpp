#include <math.h>
#include <time.h>
#include "cdatetime.h"

#if defined (Q_OS_WIN32)
#include <windows.h>
#endif

CDateTime::CDateTime() : m_dt(0.0)
{
}

CDateTime::CDateTime(const CDateTime& dt) : m_dt(dt.m_dt)
{
}

CDateTime::CDateTime(double dt) : m_dt(dt)
{
}

CDateTime::CDateTime(short nYear, short nMonth, short nDay, short nHour, short nMin, short nSec, short nMillisec)
{
	if(!_DateFromTm(nYear, nMonth, nDay, nHour, nMin, nSec,	nMillisec, m_dt))
	{
		//throw
	}
}

CDateTime::~CDateTime()
{
}

short CDateTime::GetYear()
{
	TimeStruct systm;
	_TmFromDate(m_dt, systm);

	return systm.nYear;
}

short CDateTime::GetMonth()
{
	TimeStruct systm;
	_TmFromDate(m_dt, systm);

	return systm.nMonth;
}

short CDateTime::GetDay()
{
	TimeStruct systm;
	_TmFromDate(m_dt, systm);

	return systm.nDay;
}

short CDateTime::GetHour()
{
	TimeStruct systm;
	_TmFromDate(m_dt, systm);

	return systm.nHour;
}

short CDateTime::GetMinute()
{
	TimeStruct systm;
	_TmFromDate(m_dt, systm);

	return systm.nMinute;
}

short CDateTime::GetSecond()
{
	TimeStruct systm;
	_TmFromDate(m_dt, systm);

	return systm.nSecond;
}

short CDateTime::GetMillisecond()
{
	TimeStruct systm;
	_TmFromDate(m_dt, systm);

	return systm.nMilliseconds;
}

short CDateTime::GetDayOfWeek()
{
	TimeStruct systm;
	_TmFromDate(m_dt, systm);

	return systm.nDayOfWeek;
}

short CDateTime::GetDayOfYear()
{
	//throw
	return 0;
}

CDateTime::operator double()
{
	return m_dt;
}

CDateTime::operator double*()
{
	return &m_dt;
}

CDateTime& CDateTime::operator =(const CDateTime& dtsrc)
{
	m_dt = dtsrc.m_dt;

	return *this;
}

CDateTime& CDateTime::operator =(const double dtsrc)
{
	m_dt = dtsrc;

	return *this;
}

CDateTime CDateTime::operator +(const CDateTimeSpan& span)
{
	CDateTime dt(m_dt + (double)span);

	return dt;
}

CDateTime CDateTime::operator -(const CDateTimeSpan& span)
{
	CDateTime dt(m_dt - (double)span);

	return dt;
}

CDateTimeSpan CDateTime::operator -(const CDateTime& dt)
{
	CDateTimeSpan _dt(m_dt - dt.m_dt);

	return _dt;
}

CDateTime& CDateTime::operator +=(const CDateTimeSpan span)
{
	m_dt += (double)span;

	return *this;
}

CDateTime& CDateTime::operator -=(const CDateTimeSpan span)
{
	m_dt -= (double)span;

	return *this;
}

bool CDateTime::operator ==(const CDateTime& dt)
{
	return m_dt == dt.m_dt;
}

bool CDateTime::operator !=(const CDateTime& dt)
{
	return m_dt != dt.m_dt;
}

bool CDateTime::operator <(const CDateTime& dt)
{
	return m_dt < dt.m_dt;
}

bool CDateTime::operator >(const CDateTime& dt)
{
	return m_dt > dt.m_dt;
}

bool CDateTime::operator <=(const CDateTime& dt)
{
	return m_dt <= dt.m_dt;
}

bool CDateTime::operator >=(const CDateTime& dt)
{
	return m_dt >= dt.m_dt;
}

CDateTime CDateTime::GetCurrentDateTime()
{
	short nYear = 0;
	short nMonth = 0;
	short nDay = 0;
	short nHour = 0;
	short nMinute = 0;
	short nSecond = 0;
	short nMilSec = 0;
	
#if defined(Q_OS_WIN32)
    SYSTEMTIME t;
	GetLocalTime(&t);
//	GetSystemTime(&t);
	nYear = t.wYear;
	nMonth = t.wMonth;
	nDay = t.wDay;
	nHour = t.wHour;
	nMinute = t.wMinute;
	nSecond = t.wSecond;
	nMilSec = t.wMilliseconds;

#elif defined(Q_OS_UNIX)
    // posix compliant system
    struct timeval tv;
    gettimeofday(&tv, 0);
    time_t ltime = tv.tv_sec;
	time_t ldate;
	time(&ldate);
    tm *time, *date;
	
	#if defined(QT_THREAD_SUPPORT) && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
		// use the reentrant versions of localtime() and gmtime() where available
		tm res;
		time = localtime_r(&ltime, &res);
		date = localtime_r(&ldate, &res);
	#else
		time = localtime(&ltime);
		date = localtime(&ldate);
	#endif // QT_THREAD_SUPPORT && _POSIX_THREAD_SAFE_FUNCTIONS
	
	nYear = date->tm_year + 1900;
	nMonth = date->tm_mon + 1;
	nDay = date->tm_mday;
	nHour = time->tm_hour;
	nMinute = time->tm_min;
	nSecond = time->tm_sec;
	nMinSec = tv.tv_usec / 1000;
#else
	time_t ltime;	// no millisecond resolution
	::time(&ltime);
	tm *t;
	t = localtime(&ltime);
	
	nYear = t->tm_year + 1900;
	nMonth = t->tm_mon + 1;
	nDay = t->tm_mday;
	nHour = t->tm_hour;
	nMinute = t->tm_min;
	nSecond = t->tm_sec;
	nMilSec = 0;
#endif
	
	CDateTime dt = CDateTime::CDateTime(nYear, nMonth, nDay, nHour, nMinute, nSecond, nMilSec);
	
	return dt;
}

CDateTimeSpan::CDateTimeSpan() : m_dblspan(0.0)
{
}

CDateTimeSpan::CDateTimeSpan(const CDateTimeSpan& span) : m_dblspan(span.m_dblspan)
{
}

CDateTimeSpan::CDateTimeSpan(double dblspan) : m_dblspan(dblspan)
{
}

CDateTimeSpan::CDateTimeSpan(int lDays, int nHours, int nMins, int nSecs, int nMillisecs)
	: m_dblspan(lDays + (double)nHours / 24.0 + (double)nMins / 1440.0 +
	(double)nSecs / 86400.0 + (double)nMillisecs / 86400000.0)
{
}

int CDateTimeSpan::GetDays()
{
	return (int)m_dblspan;
}

int CDateTimeSpan::GetHours()
{
	return (int)((m_dblspan - floor(m_dblspan)) * 24.0);
}

int CDateTimeSpan::GetMinutes()
{
	double dhours = m_dblspan * 24.0;

	return (int)((dhours - floor(dhours)) * 60.0);
}

int CDateTimeSpan::GetSeconds()
{
	double dmins = m_dblspan * 1440.0;

	return (int)((dmins - floor(dmins)) * 60.0);
}

int CDateTimeSpan::GetMilliseconds()
{
	double dsecs = m_dblspan * 86400.0;

	return (int)((dsecs - floor(dsecs)) * 1000.0);
}

double CDateTimeSpan::GetTotalDays()
{
	return floor(m_dblspan + 0.5);
}

double CDateTimeSpan::GetTotalHours()
{
	return floor(m_dblspan * 24.0 + 0.5);
}

double CDateTimeSpan::GetTotalMinutes()
{
	return floor(m_dblspan * 1440.0 + 0.5);
}

double CDateTimeSpan::GetTotalSeconds()
{
	return floor(m_dblspan * 86400.0 + 0.5);
}

double CDateTimeSpan::GetTotalMilliseconds()
{
	return floor(m_dblspan * 86400000.0 + 0.5);
}

CDateTimeSpan::operator double() const
{
	return m_dblspan;
}

CDateTimeSpan& CDateTimeSpan::operator =(double dblsrc)
{
	m_dblspan = dblsrc;

	return *this;
}

CDateTimeSpan& CDateTimeSpan::operator =(const CDateTimeSpan& span)
{
	m_dblspan = span.m_dblspan;

	return *this;
}

CDateTimeSpan CDateTimeSpan::operator +(const CDateTimeSpan& span)
{
	CDateTimeSpan _span(m_dblspan + span.m_dblspan);

	return _span;
}

CDateTimeSpan CDateTimeSpan::operator -(const CDateTimeSpan& span)
{
	CDateTimeSpan _span(m_dblspan - span.m_dblspan);

	return _span;
}

CDateTimeSpan& CDateTimeSpan::operator +=(const CDateTimeSpan& span)
{
	m_dblspan += span.m_dblspan;

	return *this;
}

CDateTimeSpan& CDateTimeSpan::operator -=(const CDateTimeSpan& span)
{
	m_dblspan -= span.m_dblspan;

	return *this;
}

bool CDateTimeSpan::operator ==(const CDateTimeSpan& span)
{
	return m_dblspan == span.m_dblspan;
}

bool CDateTimeSpan::operator !=(const CDateTimeSpan& span)
{
	return m_dblspan != span.m_dblspan;
}

bool CDateTimeSpan::operator <(const CDateTimeSpan& span)
{
	return m_dblspan < span.m_dblspan;
}

bool CDateTimeSpan::operator >(const CDateTimeSpan& span)
{
	return m_dblspan > span.m_dblspan;
}

bool CDateTimeSpan::operator <=(const CDateTimeSpan& span)
{
	return m_dblspan <= span.m_dblspan;
}

bool CDateTimeSpan::operator >=(const CDateTimeSpan& span)
{
	return m_dblspan >= span.m_dblspan;
}


#define MAX_TIME_BUFFER_SIZE    128         // matches that in timecore.cpp
#define MIN_DATE                (-657434L)  // about year 100
#define MAX_DATE                2958465L    // about year 9999
#define HALF_SECOND  (1.0/172800.0)
#define HALF_MILLINSEC	(1.0/172800.0/1000.0)
static int _afxMonthDays[13] =
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

bool CDateTime::_TmFromDate(double dtSrc, TimeStruct& systm)
{
	// The legal range does not actually span year 0 to 9999.
	if (dtSrc > MAX_DATE || dtSrc < MIN_DATE) // about year 100 to about 9999
		return false;

	int nDays;             // Number of days since Dec. 30, 1899
	int nDaysAbsolute;     // Number of days since 1/1/0
	int nSecsInDay;        // Time in seconds since midnight
	int nMinutesInDay;     // Minutes in day

	int n400Years;         // Number of 400 year increments since 1/1/0
	int n400Century;       // Century within 400 year block (0,1,2 or 3)
	int n4Years;           // Number of 4 year increments since 1/1/0
	int n4Day;             // Day within 4 year block
							//  (0 is 1/1/yr1, 1460 is 12/31/yr4)
	int n4Yr;              // Year within 4 year block (0,1,2 or 3)
	bool bLeap4 = true;     // TRUE if 4 year block includes leap year

	double dblDate = dtSrc; // tempory serial date

	// If a valid date, then this conversion should not overflow
	nDays = (int)dblDate;

	// Round to the second
	dblDate += ((dtSrc > 0.0) ? HALF_MILLINSEC : -HALF_MILLINSEC);

	nDaysAbsolute = (int)dblDate + 693959L; // Add days from 1/1/0 to 12/30/1899

	dblDate = fabs(dblDate);
	int nMillinsecsInDay = (int)((dblDate - floor(dblDate)) * 86400.0 * 1000.0);

	// Calculate the day of week (sun=1, mon=2...)
	//   -1 because 1/1/0 is Sat.  +1 because we want 1-based
	systm.nDayOfWeek = (int)((nDaysAbsolute - 1) % 7L) + 1;

	// Leap years every 4 yrs except centuries not multiples of 400.
	n400Years = (int)(nDaysAbsolute / 146097L);

	// Set nDaysAbsolute to day within 400-year block
	nDaysAbsolute %= 146097L;

	// -1 because first century has extra day
	n400Century = (int)((nDaysAbsolute - 1) / 36524L);

	// Non-leap century
	if (n400Century != 0)
	{
		// Set nDaysAbsolute to day within century
		nDaysAbsolute = (nDaysAbsolute - 1) % 36524L;

		// +1 because 1st 4 year increment has 1460 days
		n4Years = (int)((nDaysAbsolute + 1) / 1461L);

		if (n4Years != 0)
			n4Day = (int)((nDaysAbsolute + 1) % 1461L);
		else
		{
			bLeap4 = false;
			n4Day = (int)nDaysAbsolute;
		}
	}
	else
	{
		// Leap century - not special case!
		n4Years = (int)(nDaysAbsolute / 1461L);
		n4Day = (int)(nDaysAbsolute % 1461L);
	}

	if (bLeap4)
	{
		// -1 because first year has 366 days
		n4Yr = (n4Day - 1) / 365;

		if (n4Yr != 0)
			n4Day = (n4Day - 1) % 365;
	}
	else
	{
		n4Yr = n4Day / 365;
		n4Day %= 365;
	}

	// n4Day is now 0-based day of year. Save 1-based day of year, year number
//	int tm_yday = (int)n4Day + 1;
	systm.nYear = (unsigned short)(n400Years * 400 + n400Century * 100 + n4Years * 4 + n4Yr);

	// Handle leap year: before, on, and after Feb. 29.
	if (n4Yr == 0 && bLeap4)
	{
		// Leap Year
		if (n4Day == 59)
		{
			// Feb. 29 
			systm.nMonth = 2;
			systm.nDay = 29;
			goto DoTime;
		}

		// Pretend it's not a leap year for month/day comp.
		if (n4Day >= 60)
			--n4Day;
	}

	// Make n4DaY a 1-based day of non-leap year and compute
	//  month/day for everything but Feb. 29.
	++n4Day;

	// Month number always >= n/32, so save some loop time 
	for (systm.nMonth = (n4Day >> 5) + 1;
		n4Day > _afxMonthDays[systm.nMonth]; systm.nMonth++);

	systm.nDay = (int)(n4Day - _afxMonthDays[systm.nMonth-1]);

DoTime:
	if (nMillinsecsInDay == 0)
		systm.nHour = systm.nMinute = systm.nSecond = systm.nMilliseconds = 0;
	else
	{
		systm.nMilliseconds = nMillinsecsInDay % 1000;
		nSecsInDay = nMillinsecsInDay / 1000;
		systm.nSecond = (unsigned short)(nSecsInDay % 60L);
		nMinutesInDay = nSecsInDay / 60L;
		systm.nMinute = (int)nMinutesInDay % 60;
		systm.nHour = (int)nMinutesInDay / 60;
	}

	return true;
}

bool CDateTime::_DateFromTm(short nYear, short nMonth, short nDay,
	short nHour, short nMinute, short nSecond, short nMillisec, double& dtDest)
{
	// Validate year and month (ignore day of week and milliseconds)
	if (nYear > 9999 || nMonth < 1 || nMonth > 12)
		return false;

	//  Check for leap year and set the number of days in the month
	bool bLeapYear = ((nYear & 3) == 0) &&
		((nYear % 100) != 0 || (nYear % 400) == 0);

	int nDaysInMonth =
		_afxMonthDays[nMonth] - _afxMonthDays[nMonth-1] +
		((bLeapYear && nDay == 29 && nMonth == 2) ? 1 : 0);

	// Finish validating the date
	if (nDay < 1 || nDay > nDaysInMonth ||
		nHour > 23 || nMinute > 59 ||
		nSecond > 59)
	{
		return false;
	}

	// Cache the date in days and time in fractional days
	int nDate;
	double dblTime;

	//It is a valid date; make Jan 1, 1AD be 1
	nDate = nYear*365L + nYear/4 - nYear/100 + nYear/400 +
		_afxMonthDays[nMonth-1] + nDay;

	//  If leap year and it's before March, subtract 1:
	if (nMonth <= 2 && bLeapYear)
		--nDate;

	//  Offset so that 12/30/1899 is 0
	nDate -= 693959L;

	dblTime = (((int)nHour * 3600L) +  // hrs in seconds
		((int)nMinute * 60L) +  // mins in seconds
		((int)nSecond)) / 86400. + ((int)nMillisec) / 86400000.0;

	dtDest = (double) nDate + ((nDate >= 0) ? dblTime : -dblTime);

	return true;
}
