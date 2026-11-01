# Restoration Journal

Eigenständige fünfte C++20-App mit Qt 6.8.3 Widgets, CMake und QSQLITE.
Am 28. Oktober 2026 ausdrücklich beauftragt. Die frühere Reserve ist aktiviert.
Umfang: vier Textfelder, ein Foto, drei Statuswerte, Checkliste und PDF-Export
je eines gespeicherten Projekts. Keine weiteren Hauptfunktionen.

Workspace-Regeln gelten ergänzend. Andere Apps nur lesen. Eigene App-Kennung
`de.appfabrik.restaurierungsheft`. Keine gemeinsame Runtime oder Datenübernahme.
C++-Bezeichner und GitHub-Metadaten englisch, Dokumentation deutsch.
Oberfläche und PDF-Ausgabe unterstützen de-DE, en-US, es-ES, fr-FR, it-IT und pt-BR.
Neue sichtbare Texte vollständig übersetzen. Schema, UUIDs, Statuscodes und
Benutzerdaten bleiben sprachneutral. Sprachwahl gilt ab Neustart.
`docs/internationalization.md` beschreibt isolierte Prüfungen und Ordnerlieferung.

## Architektur

`domain` enthält Werte und Validierung. `application` enthält Service,
Repository-Vertrag und Fotoimport. `persistence` kapselt SQL. `pdf` erzeugt
PDF und übernimmt Dateien sicher, ohne Widgets oder SQL. `ui` enthält Widgets.
Ein Worker besitzt seine SQL-Verbindung. SQL, Fotoimport, PDF und Dateiarbeit
laufen außerhalb des GUI-Threads. Widgets gehören ausschließlich zum GUI-Thread.

Texte, Foto, Status und Schritte bilden einen Entwurf und eine Transaktion.
Stabile Projekt-IDs und Schritt-UUIDs, Positionslücken bleiben erhalten.
Statusschlüssel explizit zuordnen. Ungültige Daten niemals still ersetzen.
Alle Abbruchwege schützen Entwürfe mit Weiterbearbeiten als Vorauswahl.
Fotoimport nur lesend. Grenzen: 10 MiB, 6000 Pixel je Seite, 16 Millionen Pixel.

PDF verwendet einen konsistent gespeicherten Snapshot. Schrift app-lokal bündeln.
Fehlende Zeichen melden. Temporäre PDF vollständig schließen und erst dann
sicher übernehmen. Neue Ziele ohne Überschreiben veröffentlichen. Vorhandene
Ziele ausdrücklich bestätigen und vor Ersatz erneut prüfen. Bei neuen Dateien ist
der exklusive Kopierweg ohne Überschreiben erlaubt. Bestehende Ziele bleiben
atomar geschützt. Abbrechen ist die sichere Vorauswahl. Keine Exportaktion im Editor.

## Prüfen und abschließen

`source scripts/env.sh`, Formatter auf `src` und `tests`, `bash scripts/check.sh`,
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 bash scripts/check.sh asan`.
Höchstens vier parallele Build-Jobs. `.tools/env.sh` ist optional und ignoriert.
Tests nur mit synthetischen Bildern und temporären Datenbanken.
PDFs unabhängig erneut lesen, rendern und visuell prüfen. Sichtbare vollständige
Abnahme einschließlich Paket, nativen Dialogen und Neustart ohne Bildquelle.
Auf DISPLAY=:0 während des gesamten Laufs `flock /tmp/app-factory-cpp-visible-ui.lock`.
Nur eigene geprüfte Fenster und Prozesse bedienen. Roadmap unter
`/tmp/app-factory-cpp-roadmap.lock` frisch lesen und gezielt ändern.

Nach erfolgreichen Prüfungen Workspace-Changelog und lokalen App-Commit erstellen.
Kein Root-Repository, Push oder Veröffentlichung. Zuerst Microsoft Store,
später Mac App Store. Native Zielsystem-, Signierungs- und Store-Abnahmen offen
kennzeichnen, wenn nicht ausgeführt.

## Gemeinsame C++-Verbesserungen, 1. November 2026

Der öffentliche Produktname ist **Restoration Journal** in allen sechs Sprachen.
Bestehende App-Kennungen, Datenpfade und SQLite-Schemata beibehalten.
Der Nutzer bestätigt für alle fünf C++-Apps die unterstützte Systemsprache beim
Erststart mit Englisch als Fallback. Explizite Sprachwahlen bleiben erhalten.
Dies ersetzt die allgemeine Englisch-Vorgabe ausschließlich für diese C++-Reihe.

Info enthält Über die App, Datenschutz und Lizenzen mit roten Schließen-Buttons.
Die Original-Lizenztexte werden offline eingebettet. Sammlungstransfer per JSON
ist in dieser Stufe ausdrücklich beauftragt, einschließlich aller vorhandenen
Fotos und gegebenenfalls Status und Checklisten. Kein Austausch zwischen Apps.
Import ergänzt fehlende Exemplare atomar. Vorhandene Einträge bleiben erhalten.
Dateien verwenden das eigene Format in Version 1. Höchstens 10.000 Einträge,
256 MiB und 10.000 Schritte je Projekt. Fotos behalten die bisherigen Grenzen.

Auf macOS wird zuerst der Exportordner nativ gewählt, dann der Dateiname.
Nur ausgewählte Dateien und Ordner erhalten Lese-/Schreibzugriff. Neue Dateien
werden ohne Überschreiben veröffentlicht. Ohne Hardlinks darf eine Datei exklusiv
neu angelegt und kopiert werden. Nach hartem Abbruch kann dabei eine unvollständige
neue Datei zurückbleiben. Bestehende Ziele werden ausschließlich nach Bestätigung
und erneuter Prüfung atomar ersetzt. Diese Regeln gelten auch für die PDF-Ausgabe.

## Farbauswahl ab Stufe 2, 1. November 2026

Auf ausdrücklichen Nutzerauftrag wird die optionale Farbauswahl aus Yarn Box
auf alle vier C++-Apps ab Collection Showcase übertragen. Die Begrenzung auf
die bisherigen Hauptfunktionen bleibt bestehen. Farben gehören zum atomaren
Entwurf und zum JSON-Transfer. Schema 2 ergänzt das Farbfeld nach vollständiger
Prüfung und mit atomarer Migration von Schema 1. App-IDs, Datenpfade und
vorhandene Inhalte bleiben erhalten. Neue Werte verwenden #RRGGBB oder
#RRGGBBAA. Alte Exporte ohne Farbe bleiben lesbar. Alle sechs Sprachen abdecken.
Restoration Journal übernimmt die Farbe auch in den vorhandenen PDF-Export.
