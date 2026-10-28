# Prüfbericht

Stand: 28. Oktober 2026, RestaurierungsHeft 0.1.0.
Alle Testdaten sind synthetisch. Normale Benutzerdaten wurden nicht geöffnet.
Andere Portfolio-Apps wurden ausschließlich gelesen. HolzwerkProjekt blieb
auf seinem sauberen Ausgangscommit `6940968`.

## Tatsächlich verwendete Umgebung

Ubuntu 24.04.4 x64, GCC 13.3.0, Qt 6.8.3, CMake 4.4.4, Ninja 1.11.1,
clang-format 23.1.3 und clang-tidy 22.1.8 wurden lokal ermittelt.
Das dynamische Qt-SDK wurde mit GCC 10.3.1 gebaut. Die kompatible Linux-GCC-ABI
wird mit dem App-Compiler verwendet. Qt Test bestätigt Version und ABI.
SDK-Metadaten nennen libjpeg-turbo 3.1.0 und libpng 1.6.47.
Der eigene Linux-Datenpfad ist `~/.local/share/de.appfabrik.restaurierungsheft`.
Pfadtests öffnen keine normale Sammlung. Windows- und macOS-Pfade sind noch
nicht nativ abgenommen.

Für die unabhängige PDF-Prüfung wurde Poppler 24.02.0 verwendet. `pdftotext`,
`pdfinfo`, `pdffonts`, `pdfimages` und `pdftoppm` sind reine Testwerkzeuge.
Sie gehören nicht zur Produktlaufzeit. Lizenzen stehen in den Komponentenhinweisen.
Die gebündelte DejaVu Sans ist Version 2.37, geprüft mit fc-scan und SHA-256.

## Automatische Prüfungen

```bash
source scripts/env.sh
bash scripts/check.sh
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash scripts/check.sh asan
cmake --preset release
cmake --build --preset release --parallel 4
ctest --preset release
ruff check scripts
ruff format --check scripts
bash -n scripts/*.sh
```

**145 fachliche Testausführungen in neun CTest-Suiten bestehen** in Debug,
Release und unter AddressSanitizer, LeakSanitizer und UndefinedBehaviorSanitizer.
Zusätzlich meldet jede Qt-Suite Initialisierung und Abschluss. Auf dieser
Linux-Umgebung gab es keine ausgelassenen Fälle.

| Suite | Fachliche Ausführungen | Schwerpunkte |
| --- | --- | --- |
| domain | 16 | Unicode-Pflichtfelder, Textsuche, Sortierung, isolierte Datenpfade |
| repository | 17 | CRUD, stabile IDs, Duplikate, Persistenz, SQL-Texte, Sperren, Rechte und fremde Datenbanken |
| dialogs | 14 | Echte Qt-Dialoge, Abbruchwege, Löschbestätigung, Fehler, Threads, kleine Fenster |
| photo | 19 | JPEG/PNG, Inhalt, Grenzen, Beschädigung, Dateiwachstum, EXIF und BLOB-Persistenz |
| status | 10 | Standards, explizite Zuordnung, alle Wechsel, ungültige Werte und atomare Fehler |
| photo_dialogs | 8 | Fotoentwürfe, Abbruch, Fehler, veraltete Ergebnisse und zerstörte Fenster |
| checklist | 25 | UUIDs, Reihenfolge, Suche, Constraints, echte späte SQL-Fehler und vollständige Entwürfe |
| pdf | 31 | Inhalt, Schrift, Foto, Seiten, Snapshots, Dateinamen und sichere Dateioperationen |
| pdf_dialogs | 5 | Auswahl, Abbruch, Ersatz, Fehlerwiederholung, feste ID, Hintergrundarbeit und Lebensdauer |

Formatter, strenge Compilerwarnungen und clang-tidy bestehen. Ein eng begrenzter
NOLINT-Kommentar begründet die zwei benannten QString-Pfadparameter an der
Exportgrenze. Es gibt keine Sanitizer-Unterdrückungsdatei. Vorgebautes Qt,
SQLite und Bilddecoder sind selbst nicht mit den Sanitizern instrumentiert.
Offscreen-Hinweise und absichtlich ausgelöste Decoderwarnungen sind bekannte
Ausgaben der Fehlerfälle. Python-Lint und Format, Shell-Syntax, JSON und XML
wurden geprüft. PowerShell und native Zielsystemwerkzeuge wurden nicht ausgeführt.

## Speicherung und übernommene Funktionen

Echte temporäre SQLite-Datenbanken prüfen unabhängige gleichnamige Projekte,
Fotos und Schritte. Ein Trigger lässt einen späteren Schritt-INSERT scheitern,
nachdem Projektzeile und erster Schritt bereits geändert wurden. Der vollständige
Ausgangsstand bleibt erhalten. Fehlgeschlagenes Anlegen hinterlässt keine Teilzeilen.
Erneutes Speichern desselben Entwurfs gelingt nach Entfernen des Triggers.
Ein blockierter Commit und reale Datenbanksperren sind ebenfalls geprüft.

Der echte Editor erhält Texte, Foto, Status und sämtliche Schrittwerte bei
Validierungs- und Speicherfehlern. Reine Häkchen-, Status- und Fotoänderungen
lösen denselben Verwerfschutz aus. Abbrechen, Escape, Dialog- und Hauptfenster-
schließen verwenden Weiterbearbeiten als sichere Vorauswahl. Vollständig
zurückgesetzte Entwürfe schließen ohne Rückfrage. Status und Häkchen bleiben
unabhängig. Fremdschlüssel, Boolean-Regeln, eindeutige Positionen und stabile
IDs sind mit unerlaubten SQL-Änderungen geprüft.

## PDF-Inhalte und sichere Dateien

Die PDFs werden erneut mit Poppler geöffnet. Dateikopf oder Dateigröße allein
werden nicht als Funktionsnachweis verwendet.

| Prüfung | Nachweis |
| --- | --- |
| Minimales Projekt | Eine A4-Seite mit Name, Geplant und Seitenzahl |
| Alle optionalen Inhalte | Texte, PNG/JPEG und geordnete offene/erledigte Schritte |
| Alle drei Statuswerte | Jeweils mit PNG und JPEG exportiert |
| Unicode und Klartext | Umlaute, ß, Ömer, Euro, zerlegte Schreibweisen, HTML- und SQL-ähnliche Inhalte |
| Schrift | DejaVu Sans tatsächlich eingebettet, Unicode-Zuordnung vorhanden, keine externe Schrift erforderlich |
| Fehlende Zeichen | Emoji, unsichtbare Steuer-/Formatzeichen und ungültige Surrogate ausdrücklich abgelehnt |
| Fotogeometrie | Bildobjekte erneut gelesen, Pixelmaße und 2:1-Seitenverhältnis im gerenderten Ergebnis geprüft |
| Lange Inhalte | 180 Notizmarker, 3500 Zeichen langes Wort und ein Schritt mit 180 Teilmarkern über insgesamt 13 Seiten |
| Vollständigkeit | Alle Marker, lange Wörter, letzter Notiztext und letzter Schritt im erneut gelesenen Text |
| Snapshot | 100 externe vollständige SQL-Transaktionen parallel zu 150 Lesevorgängen, kein vermischter Stand |
| Projektidentität | Gleiche Namen, feste Export-ID und gespeicherter statt veralteter UI-Stand |
| Quelle entfernt | Foto-BLOB nach Quellenlöschung erfolgreich exportiert |
| Zielnamen | Unicode, Windows-Gerätenamen einschließlich hochgestellter Ziffern, leere Namen, Länge und Endung |
| Ungeeignete Ziele | Fehlende Ordner, falsche Endung, Verzeichnisse, Symlinks und Hardlink-Aliasse eigener App-Dateien |
| Ersatz | Ohne Freigabe kein Rendereraufruf, alte Datei bleibt bis zur vollständigen Erzeugung unverändert |
| Ziel geändert | Während Rendern neu angelegte, überschriebene oder mit gleichen Bytes ausgetauschte Datei erkannt |
| Späte Fehler | Teilweise temporäre Ausgabe, Renderer-/Abschlussfehler, geänderte Rechte und echter `/dev/full`-Schreibfehler |
| Daten erhalten | Alte PDF und Datenbank nach Exportfehlern bytegenau erhalten, erneuter Export erfolgreich |
| Aufräumen | Eigene temporäre Dateien nach Erfolg und Fehler entfernt |
| QSaveFile | Eigener Test belegt ungewollten Ersatz eines inzwischen erschienenen Ziels bei `commit()` |

Der QSaveFile-Nachweis begründet die kleine Plattformgrenze mit atomarem Hardlink
für neue Ziele und atomarem Ersatz bestätigter Dateien. Grenzen für bestehende
Ziele zwischen letzter Prüfung und Ersatz stehen ausdrücklich in der Architektur.
Eine exklusive Dateisperre wie auf Windows lässt sich unter Linux nicht gleichwertig
nachweisen. Der native Test gehört zur Windows-Abnahme.

Die fünf zusätzlichen echten Qt-UI-Tests prüfen auch den deaktivierten Export
ohne Auswahl, Abbruch des Qt-Dateidialogs, sichere Ersatzvorauswahl, doppelte
Auslösung, reaktionsfähige Oberfläche, Schließen während Export und zerstörte
Fenster. Kontrollierte Dialogadapter dienen den gezielten Fehlerfällen.
Native GTK-Dialoge wurden ergänzend im sichtbaren Ablauf geprüft.

## Unabhängige Geometrie und Sichtprüfung

Reproduzierbare synthetische Beispiel-PDFs entstehen durch:

```bash
source scripts/env.sh
ctest --preset dev -R '^pdf$'
python3 scripts/verify_pdf.py build/dev/verification/pdf/*.pdf
```

`minimal.pdf`, sechs vollständige Varianten und `long.pdf` liegen danach unter
`build/dev/verification/pdf`. Der zusätzliche Helfer prüft Schrifteinbettung,
A4-Maße, Seitenränder, Kopf-/Fußabstand und Wortüberschneidungen unabhängig
an den extrahierten Poppler-Koordinaten. Für das lange Dokument wurden alle
1930 Wörter auf 13 Seiten ohne Randverletzung oder Wortüberlappung geprüft.
Poppler liefert einige Ligaturen als eigene Zeichen. Inhaltsvergleiche verwenden
NFKC. Die sichtbare Schrift und vollständigen Inhalte sind erhalten.

Alle langen Seiten wurden gerendert. Insbesondere vollständiges Projekt sowie
Seiten 9, 10, 11, 12 und 13 wurden visuell angesehen. Übergänge vom langen Wort
zur Checkliste und die Schlusszeile bleiben lesbar. Die sichtbaren Bedienläufe
erzeugten zusätzlich ein siebenseitiges Dokument. Auch dessen Übergang und
Schluss sowie das Foto-PDF nach Neustart wurden visuell kontrolliert.

## Vollständige sichtbare Bedienung

```bash
source scripts/env.sh
DISPLAY=:0 python3 scripts/visible_desktop.py --wm-root .tools/desktop/root --binary build/dev/restaurierungsheft --output build/manual/dev-visible-4
```

Xephyr erhielt ein eigenes freies Display, hier :2, mit eigenem Openbox und
sichtbarem Fenster auf :0. Die gemeinsame UI-Sperre blieb über Start, Bedienung
und Abbau gehalten. Automation kontrolliert eigene Prozess- und Fenster-IDs.
Kein fremdes Display, Fenster oder Testserver wurde beendet.
Die zusätzliche lokale Openbox-Werkzeuginstallation ist ignoriert und gehört
nicht zur Produktlaufzeit. Ohne lokale Werkzeugkopie kann `--wm-root` entfallen.

Der Entwicklungsablauf besteht vollständig. Der Helfer prüft die Daten zusätzlich
über eine unabhängige lesende SQLite-Verbindung und kontrolliert Originalbildbytes.

| Sichtbarer Ablauf | Ergebnis |
| --- | --- |
| Leerzustand und Pflichtfeldfehler | Kein falscher Erfolg, Eingaben bleiben erhalten |
| Text, neue Notizzeile, PNG, Status und Schritte | Gemeinsam gespeichert |
| Native Fotoauswahl abbrechen und bestätigen | Voriger Entwurf erhalten beziehungsweise Foto übernommen |
| Schritttextsuche mit zerlegtem Umlaut | Ein Treffer, kein Treffer und leere Suche korrekt |
| Reines Häkchen, sichere Vorauswahl und Verwerfen | Gespeicherter Zustand unverändert |
| Mittlere Zeile entfernen, verbleibende umbenennen | Richtige UUID, Positionen 0 und 2 erhalten |
| Hinzufügen, Leertextfehler, abhaken und wieder öffnen | Entwurf vollständig erhalten, neue Position 3 |
| JPEG-Ersatz, Foto entfernen und verwerfen | Unabhängige Fotos gleichnamiger Projekte |
| Fertig → Geplant → Angefangen | Schritte und Foto unabhängig erhalten |
| Echter später SQL-Fehler und Wiederholung | Keine Teiländerung, erneutes Speichern gelingt |
| Nativen PDF-Dialog abbrechen | Keine Datei oder Datenbank geändert |
| Ersatz mit Enter ablehnen, danach ausdrücklich bestätigen | Sichere Vorauswahl und erfolgreiche vollständige PDF |
| Zielordner nach Bestätigungsvorbereitung schreibschützen | Verständliche Meldung, bestehende PDF bytegenau erhalten |
| Erneut an geeignetes Ziel exportieren | Erfolgreich |
| Neustart nach Entfernen beider Bildquellen | Foto, Status, Schritte, Häkchen und Reihenfolge erhalten |
| Nach Neustart exportieren | Gespeichertes JPEG und alle Projektinhalte in der PDF |
| Minimales und siebenseitiges Projekt | PDF erneut gelesen, gerendert und alle Schlussmarker geprüft |
| Löschabbruch und bestätigte Löschung | Nur zugehörige Schritte entfernt |

Die Datenbankdatei ist vor und nach jedem Export-, Abbruch- und Exportfehler-
ablauf bytegenau gleich. Eigene temporäre Exportreste bleiben nicht zurück.
Separate sichtbare Qt-Tests auf :0 prüfen 18-Punkt-Schrift bei 400 × 340 Pixeln
und bis zu 30 Schritten. Speichern und Abbrechen bleiben erreichbar.

## Eigenständiges Linux-Paket

Das Release-Paket wurde außerhalb des Projekts unter
`/tmp/restaurierungsheft-package-test-jkr45ec7/RestaurierungsHeft` sichtbar geprüft:

```bash
DISPLAY=:0 python3 scripts/visible_desktop.py --wm-root .tools/desktop/root --binary /tmp/restaurierungsheft-package-test-jkr45ec7/RestaurierungsHeft/RestaurierungsHeft.sh --output build/manual/package-visible-2 --clean-environment
python3 scripts/verify_pdf.py build/manual/package-visible-2/*.pdf
```

Derselbe vollständige Ablauf besteht aus diesem Paket. Qt-, QML-, SDK- und
externe Bibliotheksvariablen wurden entfernt, PATH auf `/usr/bin:/bin` begrenzt.
Eine eigene Fontconfig-Konfiguration stellt nur System-Noto-Schriften für den
Desktop bereit. `fc-list` findet darin keine DejaVu-Schrift. Dennoch enthält
die Paket-PDF die eingebettete DejaVu Sans mit korrektem Text und Foto.

`/proc/<pid>/maps` bestätigt Qt, ICU 73, QSQLITE, JPEG und das GTK3-Plattformthema
aus dem Paket. Der native GTK-Dialog lädt daneben den System-SVG-Decoder.
Dessen libxml2 hängt an System-ICU 74, nachgewiesen über Prozessabbild und ldd.
Das ist eine Desktopabhängigkeit, keine fehlende Qt-Paketbibliothek.
Die Nachweise liegen unter `build/manual/package-visible-2`, einschließlich
`result.json`, `loaded-libraries.json`, `process-maps.txt` und
`system-svg-dependencies.txt`.

Das abschließende Lieferarchiv unter `build/releases` enthält aktualisierte
Dokumentation und Screenshots. Launcher, Programm, Bibliotheken, Plugins und
Übersetzungen werden bytegenau gegen die sichtbar geprüfte Laufzeit verglichen.
Der Test ist ein vollständiger GUI-/PDF-Nachweis, kein bloßer Versionsaufruf.

Versionierte synthetische Belege:
[Hauptfenster](screenshots/restaurierungsheft.png),
[Editor](screenshots/checkliste.png),
[Verwerfschutz](screenshots/verwerfschutz.png),
[PDF](screenshots/pdf-projekt.png),
[langer Schritt](screenshots/pdf-langer-schritt.png),
[Ersatz](screenshots/pdf-ersetzen.png),
[Schreibfehler](screenshots/pdf-schreibfehler.png),
[große Schrift](screenshots/grosse-schrift.png),
[lange Checkliste](screenshots/lange-checkliste.png).

## Offene native und Store-Abnahmen

Windows-x64-ZIP/MSIX und macOS-App/DMG für Intel und Apple Silicon sind
vorbereitet. Sie wurden hier nicht nativ gebaut oder ausgeführt. Native Dialoge,
Dateisperren, Hardlinks, atomarer Ersatz, ACLs, Installation, Updates, Skalierung
und Tastenkürzel benötigen dort dieselbe vollständige Paketabnahme ohne SDK.
Poppler muss nur für Entwicklungstests bereitstehen.

Mac-Sandbox und user-selected.read-write sind vorbereitet, aber nicht nativ
bestätigt. Temporäre Nachbardateien und sichere Veröffentlichung innerhalb der
Freigabe sind besonders zu prüfen. Identitäten aus den Stores, Signierung,
LGPL-Konformität der konkreten Store-Distribution, Metadaten und Einreichung
stehen aus. Zielreihenfolge ist zuerst Microsoft Store, später Mac App Store.
Das Linux-Paket ersetzt diese Abnahmen nicht. Kein Push und keine Veröffentlichung.
