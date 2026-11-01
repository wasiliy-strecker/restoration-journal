# Einzel- und Sammlungs-PDFs in Restoration Journal

Das PDF-Menü bietet das ausgewählte Projekt und alle gespeicherten Projekte.
Der Gesamtexport ignoriert den Suchfilter, sortiert alphabetisch in der
App-Sprache und beginnt jedes Projekt auf einer neuen Seite. Eine gemeinsame
SQLite-Lesetransaktion sichert beide Layout-Durchläufe.

Der PDF-Aufbau entspricht Costume Planner. Titel und Status, Angaben links und
Foto rechts, Farbkachel mit Transparenz, Notiz, Arbeitsschritte als Karten,
Fortschrittsbalken und Seite-von-Gesamt sind enthalten. Lange Inhalte laufen
vollständig über weitere Seiten. DejaVu Sans Regular und Bold sind eingebettet.
Die sechs Sprachen, Datenschutzhinweise und Dokumentation sind angepasst.

Development-Build und alle 20 CTest-Suiten bestanden. Poppler prüfte Inhalte,
Schrift und Geometrie einschließlich eines 14-seitigen Berichts. Eine gerenderte
Seite wurde visuell geprüft. Auf ausdrücklichen Nutzerwunsch wurden weitere
Analyse-, Sanitizer- und native GUI-Läufe beendet. Keine Paketprüfung nachgeholt.
Der Nutzer übernimmt die weitere manuelle Abnahme. Die aktualisierte App ist
mit ihrer normalen Sammlung geöffnet. Kein Push und keine Veröffentlichung.
