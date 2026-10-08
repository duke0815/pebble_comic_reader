#pragma once
#include <pebble.h>

// Empfaengt ein PNG in Stuecken per AppMessage und dekodiert es zu einer GBitmap.

typedef struct {
  // Fortschritt in Bytes; total ist 0, solange noch kein Stueck angekommen ist.
  void (*progress)(uint32_t received, uint32_t total);
  // Fertig dekodiert. Der Empfaenger besitzt die Bitmap und gibt sie frei.
  void (*done)(GBitmap *bitmap, uint32_t png_bytes, uint32_t elapsed_ms);
  void (*failed)(const char *reason);
  // PebbleKit JS ist gestartet und nimmt Anfragen an.
  void (*phone_ready)(void);
} TransferCallbacks;

void transfer_init(TransferCallbacks callbacks);
void transfer_deinit(void);
// Fordert ein Panel vom Handy an. Eine laufende Uebertragung wird verworfen.
void transfer_request(int32_t panel_id);
