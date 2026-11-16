# Wikipedia 0.1

Enable Wikipedia in Beam68 Preferences -> Plugins, then Use or Save.
Type `wiki:Amiga` and press Enter. Select a result and press Enter to choose
it, then Enter again to read its summary in a separate native window.
Open article launches the full article using OpenURL. Close or Escape closes
the summary; Beam68 remains available through its hotkey.

Plugin Settings select the Wikipedia language, HTTPS-capable curl executable
(default C:curl), and OpenURL executable (default C:OpenURL). A running TCP/IP
stack is required. The plugin is disabled by default and searches run on Enter.
Search terms and selected article identifiers are sent to the chosen Wikipedia
language edition. HTTPS certificate checks stay enabled. No account is needed.

The MediaWiki Action API supplies at most eight results and an introductory
plain-text excerpt. Requests have time and response-size limits. Temporary
files are removed after parsing; only one summary window is allowed at once.
Latin-1 letters are retained; unsupported Unicode characters are replaced.

Code: MIT. Text: Wikipedia contributors, CC BY-SA 4.0 unless otherwise noted.
The summary includes its source article and licence links; text is shortened
and adapted for the native display. See the full article and history for attribution.

API documentation: https://www.mediawiki.org/wiki/API:Search
Extracts: https://www.mediawiki.org/wiki/Extension:TextExtracts
Licence: https://creativecommons.org/licenses/by-sa/4.0/
User-Agent: https://foundation.wikimedia.org/wiki/Policy:Wikimedia_Foundation_User-Agent_Policy
