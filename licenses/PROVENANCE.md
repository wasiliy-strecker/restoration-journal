# Herkunft der Lizenzdateien

Die Qt-Lizenztexte wurden unverändert aus der offiziellen Qt-Basis 6.8.3
übernommen. Das Verzeichnis enthält auch Hinweise für Teile dieser Basis,
die RestaurierungsHeft nicht verwendet. Das ist keine Wahl sämtlicher aufgeführter
Lizenzalternativen. Die verwendeten Bibliotheken werden unter LGPL-3.0 genutzt.

Quelle: https://download.qt.io/archive/qt/6.8/6.8.3/submodules/qtbase-everywhere-src-6.8.3.tar.xz

SHA-256: `56001b905601bb9023d399f3ba780d7fa940f3e4861e496a7c490331f49e0b80`

Das vollständige Archiv wird nicht committet. Es kann anhand der Quelle
und Prüfsumme eindeutig erneut bezogen werden. Für eine öffentliche Auslieferung muss die passende Quelle mit
Bauhinweisen zuverlässig bereitgestellt werden. Ein Link allein ersetzt keine
Prüfung der konkreten Pflichten des gewählten Auslieferungswegs.

ICU 73.2: https://github.com/unicode-org/icu/blob/release-73-2/icu4c/LICENSE

Qt-Übersetzungen 6.8.3: https://github.com/qt/qttranslations/tree/v6.8.3/LICENSES
Die Lizenzregeln für die Übersetzungsdateien stehen in der `licenseRule.json`
desselben Tags. Die GPL-Werkzeughinweise betreffen nicht automatisch die
ausgelieferten Qt-LGPL-Laufzeitbibliotheken.

Das Linux-Paket ergänzt den Lizenztext der tatsächlich beigefügten
libxcb-cursor-Version. Jedes Paket enthält zusätzlich die SPDX-Metadaten seines
konkreten Qt-SDKs. Windows-Compilerlaufzeit und macOS-Systembibliotheken müssen
bei der nativen Paketabnahme nach ihren jeweiligen Bedingungen geprüft werden.

Originaltexte von Fremdkomponenten bleiben einschließlich ihrer Sprache,
Zeichen und Formatierung unverändert.

DejaVu Sans 2.37, unveränderte TTF unter assets/fonts/DejaVuSans.ttf.
Die app-lokale Kopie wurde aus dem geprüften Portfolio-Bestand übernommen.
Die Version ist mit fc-scan 155320 / 65536, also 2.37, geprüft.
SHA-256: `7da195a74c55bef988d0d48f9508bd5d849425c1770dba5d7bfc6ce9ed848954`.
Offizielle Veröffentlichung:
https://github.com/dejavu-fonts/dejavu-fonts/releases/tag/version_2_37
Die Lizenzkopie ist unverändert unter dejavu-LICENSE.txt enthalten.

DejaVu Sans Bold 2.37, unveränderte TTF unter assets/fonts/DejaVuSans-Bold.ttf.
Die Kopie stammt aus dem vorhandenen Costume-Planner-Bestand. `fc-scan` meldet
DejaVu Sans Bold und Schriftversion 155320 / 65536, also 2.37.
SHA-256: `5c1247acef7f2b8522a31742c76d6adcb5569bacc0be7ceaa4dc39dd252ce895`.
Es gelten dieselbe offizielle Veröffentlichung und die vorhandene DejaVu-Lizenz.

Noto Emoji 3.002, unveränderte variable TrueType-Schrift mit Standardgewicht 400.
Die Originaldatei `NotoEmoji[wght].ttf` liegt unter `assets/fonts/NotoEmoji.ttf`.
Quelle mit festem Commit:
https://github.com/google/fonts/blob/8b0a1d0f5983c89bc2b93f1b5fb55f9e252744b5/ofl/notoemoji/NotoEmoji%5Bwght%5D.ttf
SHA-256: `de6c18832938afc99caf132b39d6a30a19bac7f2e812e28db2535b4608d27551`.
`fc-scan` meldet Schriftversion 196739 / 65536, also 3.002.
Die unveränderte OFL.txt desselben Commits liegt unter `noto-emoji-OFL.txt`.
Lizenz-SHA-256: `500bb1ccf43df7bbb522112f9133a52b16e1c35e809632f5d8609b179152de5b`.
