#ifndef _CDATETIME__H_
#define _CDATETIME__H_


class CDateTimeSpan
{
public:
	CDateTimeSpan();
	CDateTimeSpan(const CDateTimeSpan& span);
	CDateTimeSpan(double dblspan);
	CDateTimeSpan(int lDays, int nHours, int nMins, int nSecs, int nMillisecs);

	int GetDays();
	int GetHours();
	int GetMinutes();
	int GetSeconds();
	int GetMilliseconds();
	double GetTotalDays();
	double GetTotalHours();
	double GetTotalMinutes();
	double GetTotalSeconds();
	double GetTotalMilliseconds();

	operator double() const;
	CDateTimeSpan& operator =(double dblsrc);
	CDateTimeSpan& operator =(const CDateTimeSpan& span);
	CDateTimeSpan operator +(const CDateTimeSpan& span);
	CDateTimeSpan operator -(const CDateTimeSpan& span);
	CDateTimeSpan& operator +=(const CDateTimeSpan& span);
	CDateTimeSpan& operator -=(const CDateTimeSpan& span);
	bool operator ==(const CDateTimeSpan& span);
	bool operator !=(const CDateTimeSpan& span);
	bool operator <(const CDateTimeSpan& span);
	bool operator >(const CDateTimeSpan& span);
	bool operator <=(const CDateTimeSpan& span);
	bool operator >=(const CDateTimeSpan& span);

protected:
	double m_dblspan;
};

class CDateTime  
{
public:
	class TimeStruct
	{
	public:
		short nYear;
		short nMonth;
		short nDayOfWeek;
		short nDay;
		short nHour;
		short nMinute;
		short nSecond;
		short nMilliseconds;
	};

public:
	CDateTime();
	CDateTime(const CDateTime& dt);
	CDateTime(double dt);
	CDateTime(short nYear, short nMonth, short nDay, short nHour, short nMin, short nSec, short nMillisec);
	virtual ~CDateTime();

	short GetYear();
	short GetMonth();
	short GetDay();
	short GetHour();
	short GetMinute();
	short GetSecond();
	short GetMillisecond();
	short GetDayOfWeek();
	short GetDayOfYear();

	operator double();
	operator double*();
	CDateTime& operator =(const CDateTime& dtsrc);
	CDateTime& operator =(const double dtsrc);
	CDateTime operator +(const CDateTimeSpan& span);
	CDateTime operator -(const CDateTimeSpan& span);
	CDateTimeSpan operator -(const CDateTime& dt);
	CDateTime& operator +=(const CDateTimeSpan span);
	CDateTime& operator -=(const CDateTimeSpan span);
	bool operator ==(const CDateTime& dt);
	bool operator !=(const CDateTime& dt);
	bool operator <(const CDateTime& dt);
	bool operator >(const CDateTime& dt);
	bool operator <=(const CDateTime& dt);
	bool operator >=(const CDateTime& dt);

	static CDateTime GetCurrentDateTime();

public:
	static bool _TmFromDate(double dt, TimeStruct& systm);
	static bool _DateFromTm(short wYear, short wMonth, short wDay,
		short wHour, short wMinute, short wSecond, short wMillisec, double& dtDest);

protected:
	double m_dt;
};


#endif
