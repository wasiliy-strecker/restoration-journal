# Restoration Journal: PDF-Export mit Emojis

Emojis in Namen, Material, Ort, Notizen und Arbeitsschritten blockieren weder
Einzel- noch Sammlungs-PDFs. Die unveränderte Noto Emoji 3.002 ist einschließlich
OFL-Lizenz gebündelt. Vollständige Grapheme erhalten eine explizite einfarbige
Schriftzuweisung. Andere fehlende Zeichen erscheinen als `[U+XXXX]`. Die
Originaltexte in SQLite und JSON bleiben unverändert. Ungültige UTF-16-Daten
und echte Schreibfehler verhindern weiterhin die Übernahme der Zieldatei.

Geprüft mit `bash scripts/check.sh` und `bash scripts/check.sh asan` mit
aktivierten Address-, Leak- und UndefinedBehaviorSanitizern. Jeweils 20 Suiten
bestanden. Formatter und strenge Compilerprüfungen bestanden. clang-tidy
endet erfolgreich mit 13 Hinweisen an unveränderten Farben und Parametern.
Zehn zusätzliche Prüfungen ohne Systemschriften, unabhängiges Poppler-Lesen,
Schrift-/Geometrieprüfung und visuelle Kontrolle der Emoji-Seite bestanden.

Qt 6.8 stellt die Symbole sichtbar und druckbar dar. Das Zurückkopieren von
Emoji-Text aus PDFs bleibt eingeschränkt. JSON erhält alle Originalzeichen.
macOS und Windows wurden nicht nativ ausgeführt. Keine Store-Artefakte erstellt.
