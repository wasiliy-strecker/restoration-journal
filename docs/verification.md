# Prüfbericht

## Mac-Menüs und Systemdialogsprachen, 1. November 2026

Die App registriert auf macOS ein natives Bearbeiten-Menü mit Ausschneiden,
Kopieren, Einfügen und Alles auswählen. Die Standardaktionen verwenden die von
Qt erkannten, bereits eingebetteten Qt-Übersetzungen und Standardkürzel.
App-eigene Qt-Fenster erhalten jeweils eine Menüleiste, damit die Aktionen auch
in modalen Bearbeitungs- und Dateinamen-Dialogen verfügbar sind. Eine globale
Menüleiste dient als Rückfall für native Dialoge. Datei-, Farb-, Schrift- und
Meldungsdialoge werden durch den Menühelfer nicht mit Qt-Kindwidgets erweitert.

Qt-Textaktionen betreffen ausschließlich das aktuell fokussierte, sichtbare und
aktivierte Textfeld. Schreibgeschützte Inhalte bleiben unverändert. Native
Mac-Dialoge nutzen die Qt-/AppKit-Weiterleitung und deren eigene Aktionsprüfung.
Unter Linux und Windows wird der Helfer im normalen App-Start nicht aktiviert.
Die neuen Tests instanziieren ihn dort gezielt zur Prüfung der gemeinsamen Logik.

Die Mac-Konfiguration deklariert Deutsch, Englisch, Spanisch, Französisch,
Italienisch und brasilianisches Portugiesisch mit Englisch als Fallback.
Native Mac-Dateidialoge folgen ausdrücklich der macOS-Sprachwahl. Die eigene
Sprachwahl der App bleibt unabhängig und gilt weiterhin nach einem Neustart.
Es werden keine Apple-Spracheinstellungen geschrieben und keine App-Kennungen,
Datenpfade, Datenbankinhalte oder Exportformate geändert.

Zusätzliche Regressionstests: `native_edit_menus` und `macos_localizations`.
Sie prüfen Textaktionen, Unicode, Undo nach Ausschneiden, Fokuswechsel, modale
Dateinamen-Eingabe, schreibgeschützte Felder, normale Tastenkürzel, wiederholtes
Öffnen, unveränderte Dateidialoge sowie die Katalog- und Plist-Sprachzuordnung.
Die gemeinsamen Tests prüfen die sechs Sprachen einschließlich der vorhandenen
Systemsprachwahl, expliziten Präferenzen und des Englisch-Fallbacks.

### Ausgeführte Linux-Prüfung

Alle 19 CTest-Suiten bestehen in Development und unter AddressSanitizer,
LeakSanitizer und UndefinedBehaviorSanitizer für den eigenen Code.
Formatter, strikte Compilerwarnungen, clang-tidy und die Python-Prüfungen bestehen.
Die Menütests bestehen zusätzlich mit xcb und GTK3 auf einem eigenen X11-Display.
Der vollständige Bedienablauf der aktuellen Development-Datei in einer separaten
Testlaufzeit mit eigener Qt-Laufzeit besteht einschließlich Speichern und Neustart.
Repräsentative Aufnahmen wurden visuell geprüft. Alle Daten sind synthetisch.
Frühere Pakete und die normale Nutzersammlung bleiben erhalten.

Nachweise: `build/mac-fix-dev.log`, `build/mac-fix-asan.log`,
`build/mac-fix-native/native-edit-menus.log`,
`build/mac-fix-journey/` und `build/mac-fix-native/result.json`.
Letztere Datei hält die Prüfsumme der tatsächlich geprüften ausführbaren Datei fest.
Die Testlaufzeit liegt unter `build/mac-fix-runtime/`. Kein Lieferarchiv.

Der Bedienablauf prüft zusätzlich native PDF-Dateiauswahl, Ersatz, Abbruch,
Schreibfehler, minimalen und mehrseitigen Export sowie Neustart ohne Bildquelle.
Poppler hat die PDFs erneut gelesen und gerendert. Die normale Projektseite
und die letzte Seite des siebenseitigen Exports wurden visuell geprüft.

### Noch offene native Mac-Abnahme

Die Linux-Prüfung ersetzt keine Ausführung in Cocoa oder der Apple-Sandbox.
Auf einem Mac mit isolierten synthetischen Daten sind zusätzlich zu prüfen:

- In den vorhandenen nativen Dateidialogen funktionieren ⌘X, ⌘C, ⌘V und ⌘A im
  Suchfeld beziehungsweise in der Pfadeingabe. Der tatsächlich fokussierte
  Dialog erhält die Aktion. Hintergrundentwürfe bleiben unverändert.
- Im Bearbeitungsdialog und Exportdateiname funktionieren dieselben Aktionen
  per Menü und Tastatur. Schreibgeschützte Ansichten erlauben keine Änderungen.
- Erststart und Dateidialoge in allen sechs Systemsprachen, Englisch-Fallback
  ohne unterstützte macOS-Sprachpräferenz und Neustart nach expliziter App-Wahl.
  Eine abweichende App-Sprachwahl überschreibt die Mac-Dialogsprache nicht.
- Import, Export, bestätigter Ersatz, Abbruch, Dateinamen mit Unicode und Neustart
  mit der Sandbox-Testapp funktionieren. Fotoimport gehört ab Stufe 2 dazu.
  Restoration Journal prüft zusätzlich den PDF-Export.
- Beenden per ⌘Q, App-Menü und Fensterschließen schützt ungespeicherte Entwürfe.

Für diese Mac-Prüfung wurde kein Release-Paket erzeugt und keine Store-Abnahme
behauptet. Referenz: [Qt 6.8 zu macOS-Menüs und Internationalisierung](https://doc.qt.io/qt-6.8/macos-issues.html).

## Sammlungstransfer und gemeinsame Oberfläche, 1. November 2026

Restoration Journal übernimmt die Verbesserungen von Board Game Shelf bis Commit `11245ea`.
Buttons sind neu angeordnet. Alle Tabellenspalten bieten auf- und absteigende
Sortierung, ohne die ausgewählte Eintrags-ID zu verlieren. Info enthält drei
getrennte Offline-Fenster mit roten Schließen-Buttons. Der öffentliche englische
Name bleibt in allen sechs Sprachen gleich. Die Systemsprache beim Erststart
mit Englisch als Fallback ist ausdrücklich bestätigt. Datenpfade, App-Kennungen,
Datenbankschema und gespeicherte Sprachwahlen bleiben erhalten.

JSON-Dateien sichern alle gespeicherten Inhalte einschließlich Originalfotos.
Ein konsistenter Lesesnapshot enthält auch die in der Tabelle absichtlich nicht
geladenen Foto-BLOBs. Import ergänzt fehlende Exemplare atomar, berücksichtigt
die Anzahl identischer Exemplare und verändert keine vorhandenen Einträge.
Neue Arbeitsschritte erhalten bei UUID-Kollisionen neue Kennungen. Reihenfolge
und Häkchen bleiben erhalten. Der Worker importiert den bestätigten Snapshot,
auch wenn die Quelldatei zwischenzeitlich geändert wird.

Die vollständigen 16 CTest-Suiten bestehen in Development, Release und mit
AddressSanitizer, LeakSanitizer und UndefinedBehaviorSanitizer. Formatter,
strikte Compilerwarnungen, clang-tidy, Ruff, Shell- und XML-Prüfung bestehen.
Die vorgebauten Qt-Bibliotheken sind selbst nicht mit Sanitizern instrumentiert.
Nach den letzten Layoutkorrekturen wurden die betroffenen UI-, Übersetzungs-
und Dialogsuiten in allen drei Konfigurationen zusätzlich erfolgreich geprüft.

Native X11-Prüfungen auf eigenen Xvfb-Bildschirmen verwenden ausschließlich
synthetische Daten. Geprüft sind alle sechs Sprachen, kleine Fenster,
Tastaturbedienung der Info-Menüs, rote Schließen-Buttons, Sortierung mit
beibehaltener Auswahl, vollständiger Sammlungstransfer und die vorhandenen
Foto-/Projektabläufe mit Neustart ohne Bildquelle.

Auch die eigenständigen Linux-Pakete mit eigener Qt-Laufzeit bestehen den
vollständigen Bedienablauf. Native GTK-Dateidialoge prüfen Export bei aktiver
Suche, Importabbruch, bestätigten Import mit Fotos, wiederholten Import ohne
zusätzliche Duplikate und Neustart. Bei Checklisten werden zusätzlich kollidierende
Schrittkennungen geprüft. Laufzeitnachweise kontrollieren die tatsächlich geladenen
Qt-, SQLite- und JPEG-Bibliotheken. Die Pakete werden ausschließlich als Ordner
mit `--stage-only` erzeugt. Vorherige Testpakete bleiben erhalten.

Die Tests simulieren fehlende Hardlinks, späte Dateikollisionen und Schreibfehler.
Neue Dateien werden dann exklusiv angelegt. Der Kopierweg ist nicht atomar und
kann bei hartem Prozessabbruch eine unvollständige neue Datei hinterlassen.
Bestehende Dateien werden ausschließlich nach Bestätigung und erneuter Prüfung
atomar ersetzt. App-Daten und Dateiverweise sind als Exportziel geschützt.

Native Windows- und macOS-Ausführung, Apple-Sandbox, Signierung und ein physischer
exFAT-Datenträger bleiben offen. Linux-Prüfungen sind keine Store-Freigabe.

Prüfbefehle und lokale Nachweise:

```bash
bash scripts/check.sh
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash scripts/check.sh asan
cmake --build --preset release --parallel 4
ctest --preset release
```

Vollständige Protokolle liegen unter `build/port-verified-dev.log`,
`build/port-verified-asan.log` und `build/port-release-check.log`.
Die abschließenden UI-Prüfungen stehen in `build/port-final-*.log`.
Native Belege und Aufnahmen liegen in `build/port-final-native/`,
`build/port-package-journey/` und `build/port-final-package-transfer/`.
Die abschließenden Testpakete liegen in `build/port-package-final/`.

Die PDF-Ausgabe behält den gespeicherten Projektsnapshot, die eingebettete Schrift
und alle vorhandenen Inhalte. Auch PDF verwendet auf macOS die native Ordnerwahl
und unterstützt neue Ziele ohne Hardlinks. PDFs werden unabhängig mit Poppler
auf Textinhalt, Bilder, Schrift und Seitenumbrüche geprüft und gerendert.
Die langen Texte bleiben auch über mehrere Seiten vollständig erhalten.


Stand: 29. Oktober 2026, RestaurierungsHeft 0.1.0.
Alle Testdaten sind synthetisch. Normale Benutzerdaten wurden nicht geöffnet.

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

**203 fachliche Qt-Testausführungen und die Katalogprüfung bestehen** in Release
und unter AddressSanitizer, LeakSanitizer und UndefinedBehaviorSanitizer.
Das sind zehn Qt-Suiten und elf erfolgreiche CTest-Einträge.
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
| pdf | 37 | Inhalt, Schrift, Foto, Seiten, Snapshots, Dateinamen und sichere Dateioperationen |
| pdf_dialogs | 5 | Auswahl, Abbruch, Ersatz, Fehlerwiederholung, feste ID, Hintergrundarbeit und Lebensdauer |
| i18n | 52 | Sechs Sprachen, Formatparameter, regionale Formate, Spracheinstellungen, Worker und übersetzte Dialoge |

Formatter, strenge Compilerwarnungen und clang-tidy bestehen. Ein eng begrenzter
NOLINT-Kommentar begründet die zwei benannten QString-Pfadparameter an der
Exportgrenze. Es gibt keine Sanitizer-Unterdrückungsdatei. Vorgebautes Qt,
SQLite und Bilddecoder sind selbst nicht mit den Sanitizern instrumentiert.
Offscreen-Hinweise und absichtlich ausgelöste Decoderwarnungen sind bekannte
Ausgaben der Fehlerfälle. Python-Lint und Format, Shell-Syntax, JSON und XML
wurden geprüft. PowerShell und native Zielsystemwerkzeuge wurden nicht ausgeführt.

## Internationalisierung vom 29. Oktober 2026

181 eigene Meldungen je Sprache und passende Qt-Basisübersetzungen sind für
de-DE, en-US, es-ES, fr-FR, it-IT und pt-BR eingebettet. Katalogschlüssel,
Formatparameter, regionale Formate und atomare Spracheinstellungen werden
geprüft. Die gewählte Sprache gilt ab Neustart. Eigene UI, Fehler und PDF-Felder
sind übersetzt. Produktidentität, Schema und gespeicherte Nutzertexte bleiben erhalten.

Zwölf echte Prozesse aus dem eigenständigen Paket bestehen den vollständigen
Ablauf in sechs Sprachen einschließlich Neustart. Fotos, alle Statuswechsel,
leere Schritte, doppelte Schritttexte mit unabhängigen UUIDs, Positionslücken,
Verwerfen, echte SQLite-Sperren, Fehlerwiederholung und Sprachwechsel sind
nachgewiesen. 24 echte Aufnahmen in 1920 × 1080 wurden visuell geprüft.

Jede Sprache exportiert ein gespeichertes Projekt über den nativen Dialog.
Abbruch, sichere Ersatzvorauswahl und bestätigter Ersatz werden bedient.
Poppler prüft Text, eingebettete DejaVu-Schrift, Fotoobjekte und A4-Seiten
unabhängig und rendert sämtliche Seiten. Alle sechs Projektseiten wurden visuell
geprüft. Unicode-Ligaturen werden im Textvergleich mit NFKC normalisiert.
Die Datenbank bleibt bei jedem Export bytegenau unverändert.

Zusätzlich besteht der bisherige vollständige deutsche Paketablauf mit
Schreibfehler, Wiederholung, Quellenlöschung, Neustart, minimaler PDF und einem
siebenseitigen Dokument. Foto, Checkliste, Umbrüche und Schlussseite wurden
visuell geprüft. Alle Laufzeitdateien, Quellstände und Nachweise werden über
SHA-256 zugeordnet. Sechs Store-Textfassungen und eigene Restaurierungsmotive
liegen vor. Es werden ausschließlich Ordner geliefert.

Aktuelle Nachweise liegen in den ignorierten Verzeichnissen:

```text
build/international-release-check.log
build/international-asan-check.log
build/international-captures-final-v2
build/international-legacy-gui-final
```

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
ctest --preset release -R '^pdf$'
python3 scripts/verify_pdf.py build/release/verification/pdf/*.pdf
```

`minimal.pdf`, sechs vollständige Varianten und `long.pdf` liegen danach unter
`build/release/verification/pdf`. Der zusätzliche Helfer prüft Schrifteinbettung,
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

## Vollständige echte Bedienung

Der bisherige vollständige deutsche Ablauf wurde am 29. Oktober aus dem neuen
Paket erneut ausgeführt. Ein eigenes Xvfb-Display mit eigenem Openbox bleibt
vollständig vom Nutzerdesktop getrennt. Die Automation kontrolliert ausschließlich
eigene Prozess- und Fenster-IDs. Es werden nur synthetische Daten verwendet.

Der Helfer prüft den Datenbestand zusätzlich über eine unabhängige lesende
SQLite-Verbindung und kontrolliert Originalbildbytes. Der Ablauf unter
`build/international-legacy-gui-final` besteht einschließlich nativer Dialoge,
PDF-Schreibfehler, sicherem Dateiersatz und Neustart.

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
Die Qt-Dialogtests prüfen zusätzlich große Schrift und kleine Fenster.
Speichern und Abbrechen bleiben erreichbar.

## Eigenständiges Linux-Paket

Geprüfte Laufzeit:

```text
build/staging/international-20261029/restaurierungsheft-0.1.0-linux-x64/RestaurierungsHeft.sh
```

Qt-, QML-, SDK- und externe Bibliotheksvariablen wurden entfernt, PATH auf
`/usr/bin:/bin` begrenzt. Eine eigene Fontconfig-Konfiguration stellt im deutschen
Gesamtablauf nur System-Noto-Schriften für den Desktop bereit. Die Paket-PDF
enthält dennoch die eingebettete DejaVu Sans mit korrektem Text und Foto.
`/proc/<pid>/maps` bestätigt Qt, ICU 73, QSQLITE, JPEG und das GTK3-Plattformthema
aus dem Paket. Der System-SVG-Decoder des nativen GTK-Dialogs bleibt eine
Desktopabhängigkeit. `loaded-libraries.json` und die Prozessabbilder enthalten
die konkreten Herkunftsnachweise.

Die internationale Lieferung besteht aus einem Ordner ohne Archiv. Launcher,
Programm, Bibliotheken, Plugins, Übersetzungen und Quellstand werden bytegenau
gegen die geprüfte Laufzeit verglichen. Das Manifest nennt den lokalen Commit.
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

Windows-x64-Paketierung/MSIX und macOS-App/DMG für Intel und Apple Silicon sind
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

## Farbauswahl pro Eintrag, 1. November 2026

Die optionale Farbauswahl aus Yarn Box ist in dieser App umgesetzt. Der Qt-
Farbwähler bietet Palette, eigene Farben und Transparenz. Die Auswahl wird erst
mit dem gesamten Entwurf gespeichert. Farbkachel, Hexcode, Tab-Reihenfolge,
Entfernen, Suche und Sortierung wurden geprüft.

`bash scripts/check.sh` besteht mit 17 CTest-Suiten, clang-format,
Compilerwarnungen als Fehlern und clang-tidy ohne neue Meldungen.
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash scripts/check.sh asan`
besteht ebenfalls mit allen 17 Suiten. Die neuen Farbtests prüfen RGBA-Bytes,
Migration echter Schema-1-Fixtures einschließlich Foto, Status und Schritten,
Rollback einer gesperrten Migration und erneuten Versuch, atomare Schreibfehler,
Altimporte ohne Farbe, Importunterschiede und Wiederholungen sowie Editor-
und Tabellenabläufe in sechs Sprachen. Alle Prüfdaten sind synthetisch.

Zusätzlich bestehen `color_tests` und `collection_features_tests` mit xcb und
GTK3 auf einem eigenen sichtbaren Xephyr-Bildschirm. Das eigenständige Linux-
Paket unter `build/manual/color-package/restaurierungsheft-0.1.0-linux-x64/`
besteht `scripts/color_gui_smoke.py` und den vollständigen bisherigen
`scripts/gui_smoke.py`-Ablauf mit bereinigter Entwicklungsumgebung. Die neue
Prüfung öffnet den nativen Farbpicker per Tab und Leertaste, bestätigt Weiß mit
dem nativen Auswahlbutton, bricht eine weitere Auswahl ab, entfernt die Farbe
und prüft beide gespeicherten Zustände nach zwei Neustarts. Der bisherige
Ablauf prüft außerdem Fotoimport, Status, Checkliste, Fehler und Neustart ohne
Bildquelle. Die normalen Nutzerdaten werden von Tests nicht geöffnet.

Nachweise liegen unter `build/manual/colors/`, `build/manual/color-package-native/`
und `build/manual/color-package-workflow/`. Native Windows- und macOS-
Prüfungen wurden in dieser Linux-Umgebung nicht ausgeführt. Keine Store-
Veröffentlichung, kein Upload und kein Lieferarchiv.

Der PDF-Test extrahiert die Farbangabe in allen sechs Sprachen mit pdftotext.
Die Berichte werden unabhängig mit pdftoppm gerendert. Der deutsche Bericht
mit transparenter Farbkachel und Hexwert wurde visuell geprüft. Auch die
nativen PDF-Dateidialoge des eigenständigen Pakets bestehen die Abnahme.
