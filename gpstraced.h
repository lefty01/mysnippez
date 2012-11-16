
#ifndef _GPSTRACED_H_
#define _GPSTRACED_H_

//const char _track_name[] = "$TRACK___________NAME$";

const char xml_head_1[] =						\
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"			\
	"<gpx\n"							\
	"  version=\"1.0\"\n"						\
	"  creator=\"gpstraced v0.1 by andreas loeffler, exitzero.de\"\n" \
	"  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"	\
	"  xmlns=\"http://www.topografix.com/GPX/1/0\"\n"		\
	"  xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n" \
	"<trk>\n"							\
	"<name>";

const char xml_head_2[] =		\
	"</name>\n"			\
	"<trkseg>\n";

const char xml_end[] =				\
	"</trkseg>\n"				\
	"</trk>\n</gpx>\n";
#endif
