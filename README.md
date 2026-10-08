# Comic Reader für Pebble Time 2 (emery)

Liest Comics Panel für Panel am Handgelenk. Die Uhr speichert keine Comics,
sie bekommt jedes Panel bei Bedarf als PNG vom Handy (PebbleKit JS).

## Stand: Schritt 1, Prototyp

- `src/pkjs/index.js` schickt ein fest eingebautes Test-PNG (`src/pkjs/test_panel.js`)
  in Stücken per AppMessage an die Uhr. Die Stückgröße richtet sich nach
  `app_message_inbox_size_maximum()`, das die Uhr bei der Anfrage mitschickt.
- `src/c/transfer.c` setzt die Stücke zusammen und dekodiert sie mit
  `gbitmap_create_from_png_data()`.
- `src/c/main.c` zeigt das Panel scrollbar an (UP/DOWN) und blendet Größe,
  Dauer und freien Heap ein. SELECT lädt das Panel neu, um die Messung zu wiederholen.

### Protokoll

| Richtung | Keys | Bedeutung |
|---|---|---|
| Handy → Uhr | `READY` | PebbleKit JS läuft und nimmt Anfragen an |
| Uhr → Handy | `REQUEST`, `INBOX_SIZE` | Panel anfordern |
| Handy → Uhr | `TOTAL_SIZE`, `OFFSET`, `DATA` | ein Stück des PNG, der Reihe nach |
| Handy → Uhr | `ERROR` | Abbruch mit Text |

## Test-Panel neu erzeugen

```sh
python3 tools/make_test_panel.py            # synthetisches Testbild
python3 tools/make_test_panel.py bild.png   # eigenes Bild
```

Das Skript skaliert auf 200 px Breite, dithert per Floyd-Steinberg auf die
64-Farben-Palette und schreibt `src/pkjs/test_panel.js`.

## Bauen

CloudPebble mit GitHub-Sync oder lokal mit `pebble build`.
