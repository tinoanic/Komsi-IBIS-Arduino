#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "Parola_Fonts_data.h"
#include "TimerOne.h"
#include <Wire.h>
#include <String.h>

#define MAX_DEVICES 16 // Number of modules connected
#define CLK_PIN 8      // SPI SCK pin on UNO
#define DATA_PIN 11    // SPI MOSI pin on UNO
#define CS_PIN 12      // connected to pin 10 on UNO
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

#define DEBUG 0

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

String aktuelle_haltestelle;
String linie_to_send;
String leerText = "Betriebsfahrt";
String inputString = "";
String alteHst = "";
boolean stringComplete = false;
String alteLinie = "";
String line_from_serial;
String haltestelle_from_serial;
String linien_text;
String haltestellen_text;
String linien_text_alt;
String haltestellen_text_alt;
String nextStopText = "\x09 Nächster Halt \x09";
int anzeigeZyklus = 1; // 1: Pause (Anzeige nur nach Start) 2: Linie 3: Text Nächster Halt 4: Haltestelle 5: Leer -> Betriebsfahrt
int charCounter;
int wechselIntervall = 3;
const byte numChars = 64;
char receivedChars[numChars];
bool firstStart = true;

void setup()
{
  Timer1.initialize(wechselIntervall * 1000000);
  Timer1.attachInterrupt(displayWechsel);
  // serielle Kommunikation vorbereiten bzw. starten
  Serial.begin(115200);
  Serial.flush();
  nextStopText = utf8ascii(nextStopText);
  P.begin(2);
  P.setZone(0, 0, 12);
  P.setZone(1, 13, 15);
  P.setFont(ExtASCII);
  P.setIntensity(2);
  haltestellen_text = ("I.B.I.S. v1.23 Connect KOMSI");
  P.setInvert(1, true);
  linien_text = "\x07";
}

void loop()
{

  if (stringComplete)
  {
    analyzeReceivedString();
  }
  else
  {
    recvSerialWithEndMarker();
  }
  animateTheDisplay();
}

void displayWechsel()
{
  if (stringComplete || firstStart)
  {
    switch (anzeigeZyklus)
    {
    case 1: // Pause-Anzeige
      haltestellen_text = "\x07 Pause \x07";
      break;
    case 2: // Update Linienanzeige
      linien_text = linie_to_send;
      anzeigeZyklus = 3;
      break;
    case 3: // Anzeige Text Nächster Halt
      haltestellen_text = nextStopText;
      P.displayClear(0);
      anzeigeZyklus = 4;
      break;
    case 4: // Anzeige Haltestellenname
            // Ersetze U und S
      if (aktuelle_haltestelle.startsWith("S "))
      {
        aktuelle_haltestelle.replace("S ", "\xff ");
      }
      if (aktuelle_haltestelle.startsWith("U "))
      {
        aktuelle_haltestelle.replace("U ", "\xfe ");
      }
      if (aktuelle_haltestelle.startsWith("S+U "))
      {
        aktuelle_haltestelle.replace("S+U", "\xff+\xfe");
      }
      haltestellen_text = aktuelle_haltestelle;
      break;
    case 5: // Anzeige Betriebsfahrt
      haltestellen_text = leerText;
      break;
    default:
      haltestellen_text = "FEHLER";
      break;
    }
  }
}

void analyzeReceivedString()
{
  // Zerlege den empfangenen String in seine Einzelteile

  String split = inputString;
  line_from_serial = getValue(split, ',', 1);        // Linie kurz
  haltestelle_from_serial = getValue(split, ',', 6); // Haltestellenname
  split = "";
  inputString = "";
  stringComplete = false;

  //Überprüfe, ob sich die Haltestelle oder die Linie geändert hat und fixe falsche Linien
  // Haltestelle zuerst

  if (haltestelle_from_serial != alteHst)
  {
    aktuelle_haltestelle = haltestelle_from_serial;
    alteHst = haltestelle_from_serial;
    if (aktuelle_haltestelle != "")
    {
      anzeigeZyklus = 3;
    }
    else
    {
      anzeigeZyklus = 5;
    }
  }

  // Linie

  if (line_from_serial != alteLinie)
  {
    // Formatierung für bestimmte Linien
    if (line_from_serial == "10")
    {
      linie_to_send = "X10";
    }
    else if (line_from_serial == "19")
    {
      linie_to_send = "M19";
    }
    else if (line_from_serial == "29")
    {
      linie_to_send = "M29";
    }
    else if (line_from_serial == "0")
    {
      linie_to_send = "\x08";
    }
    else
    {
      linie_to_send = line_from_serial;
    }
    anzeigeZyklus = 2;
    alteLinie = line_from_serial;
  }
}

void animateTheDisplay()
{
  if (P.displayAnimate())
  {
    if ((linien_text != linien_text_alt) || (haltestellen_text_alt != haltestellen_text) || (haltestellen_text.length() > 17 && P.getZoneStatus(0) == true))
    {
      if (P.getZoneStatus(0))
      {
        
        const char *cStyleTextString = haltestellen_text.c_str();

        if (haltestellen_text.length() > 17)
        {
          P.displayClear(0);
          P.displayZoneText(0, cStyleTextString, PA_CENTER, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        }
        else
        {
          P.displayClear(0);
          P.displayZoneText(0, cStyleTextString, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
        }
        haltestellen_text_alt = haltestellen_text;
      }
      if (P.getZoneStatus(1))
      {
        const char *cStyleLineString = linien_text.c_str();
        P.displayZoneText(1, cStyleLineString, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
        linien_text_alt = linien_text;
      }
    }
  }
}

void recvSerialWithEndMarker()
{
  if (!stringComplete)
  {
    static byte ndx = 0;
    char endMarker = ';';
    char rc;

    while (Serial.available() > 0)
    {
      rc = Serial.read();

      if (rc != endMarker)
      {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars)
        {
          ndx = numChars - 1;
        }
      }
      else
      {
        receivedChars[ndx] = '\0'; // terminate the string
        inputString = receivedChars;
        stringComplete = true;
        ndx = 0;
      }
    }
  }
}




String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  // return found>index ? data.substring(strIndex[0], strIndex[1]) : "elufzg";
  return data.substring(strIndex[0], strIndex[1]);
}

// ****** UTF8-Decoder: convert UTF8-string to extended ASCII *******
static byte c1; // Last character buffer

// Convert a single Character from UTF8 to Extended ASCII
// Return "0" if a byte has to be ignored
byte utf8ascii(byte ascii)
{
  if (ascii < 128) // Standard ASCII-set 0..0x7F handling
  {
    c1 = 0;
    return (ascii);
  }

  // get previous input
  byte last = c1; // get last char
  c1 = ascii;     // remember actual character

  switch (last) // conversion depending on first UTF8-character
  {
  case 0xC2:
    return (ascii);
    break;
  case 0xC3:
    return (ascii | 0xC0);
    break;
  case 0x82:
    if (ascii == 0xAC)
      return (0x80); // special case Euro-symbol
  }

  return (0); // otherwise: return zero, if character has to be ignored
}

// convert String object from UTF8 String to Extended ASCII
String utf8ascii(String s)
{
  String r = "";
  char c;
  for (int i = 0; i < s.length(); i++)
  {
    c = utf8ascii(s.charAt(i));
    if (c != 0)
      r += c;
  }
  return r;
}

// In Place conversion UTF8-string to Extended ASCII (ASCII is shorter!)
void utf8ascii(char *s)
{
  int k = 0;
  char c;
  for (int i = 0; i < strlen(s); i++)
  {
    c = utf8ascii(s[i]);
    if (c != 0)
      s[k++] = c;
  }
  s[k] = 0;
}
