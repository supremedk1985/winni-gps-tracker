#include <Arduino.h>

#include "utils.h"

// --- Datum/Zeit formatieren ---
String fmtUTC(const String &utcRaw) {
  if (utcRaw.length() < 13) return utcRaw;
  String dd = utcRaw.substring(0,2);
  String mm = utcRaw.substring(2,4);
  String yy = utcRaw.substring(4,6);
  String hh = utcRaw.substring(7,9);
  String mi = utcRaw.substring(9,11);
  String ss = utcRaw.substring(11,13);
  return "20" + yy + "-" + mm + "-" + dd + " " + hh + ":" + mi + ":" + ss + " UTC";
}

String urlEncode(const String &s)
{
  String encoded = "";
  char c;
  char bufHex[4];
  for (int i = 0; i < s.length(); i++)
  {
    c = s.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      encoded += c;
    }
    else if (c == ' ')
    {
      encoded += "%20";
    }
    else if (c == '\n')
    {
      encoded += "%0A"; // Zeilenumbruch
    }
    else
    {
      snprintf(bufHex, sizeof(bufHex), "%%%02X", (unsigned char)c);
      encoded += bufHex;
    }
  }
  return encoded;
}