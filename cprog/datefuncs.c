#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

/*
 * Shamelessly stolen from
 * http://pleac.sourceforge.net/pleac_cposix/datesandtimes.html
 */

//double get_date_interval_value(const char* intvltype);
//double from_epoch(const char* intvltype, time_t tv);

double get_date_interval_value(const char* intvltype)
{
  double interval = 0.0;

  // What, no lookup table ;) ?
  switch (*intvltype)
  {
    case 'd' : interval = strncasecmp(intvltype, "day", 3) == 0 ? 86400.0 : 0.0; break;
    case 'h' : interval = strncasecmp(intvltype, "hou", 3) == 0 ? 3600.0 : 0.0; break;
    case 'm' : interval = strncasecmp(intvltype, "min", 3) == 0 ? 60.0 : 0.0; break;
    case 's' : interval = strncasecmp(intvltype, "sec", 3) == 0 ? 1.0 : 0.0; break;
    case 'w' : interval = strncasecmp(intvltype, "wee", 3) == 0 ? 604800.0 : 0.0; break;
  }

  return interval;
}

void frac_to_hms(double frac, int* hour, int* min, int* sec)
{
  int seconds = floor(frac * 86400.);

  *hour = seconds / 3600;
  *min = (seconds - *hour * 3600) / 60;
  *sec = (seconds - (*hour * 3600 + *min * 60));
}

double from_epoch(const char* intvltype, time_t tv)
{
  double interval = get_date_interval_value(intvltype);
  return (interval > 0.0) ? tv / interval : 0.0;
}


void tics2comment(time_t old, time_t new, char *buf)
{
	time_t difference = difftime(old, new);
	int hour, min, sec;

	double frac = ({ double days = from_epoch("day", difference); days - floor(days); });

	frac_to_hms(frac, &hour, &min, &sec);

	sprintf(buf, "%d days, %02d:%02d:%02d",
		(int) from_epoch("day", difference),
		hour, min, sec);
}

