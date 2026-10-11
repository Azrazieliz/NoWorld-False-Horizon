# Gate 01 → Gate 00 → Gates 11 & 12 — Luminous Valentine v3, 2026-10-11

**Status: complete SOURCE HANDOFF PACKAGE, not approved ARTIFACT_READY or Unreal import.** Do not modify frozen Foundation, saves, draft/unmerged PR #16.

## Component B: 3D authoring sources (verified)

Located on the authorized Windows production machine under:
`D:\Tools\GameAssetPipeline\KoikatsuParty\handoffs\GATE00_LUMINOUS_V3_Koikatsu_3D_Source_and_Registry_Intake_20261011.zip`

- Bytes: **95,313,987**
- SHA-256: `0fd7d10d09b925bbd27ae26771a3559d6664171984c1c916e0cc0bc96c62f782`
- Archive: **19 asset files**, manifest, explanatory handoff; ZIP CRC and per-file SHA hashes verified.
- Primary user-created *Koikatsu Party embedded-data character cards* (three separate, not 3 Unreal meshes):
  - `03_KK_PARTY_CARDS/PRIMARY_USER_SAVED/CLOTHED.png`: SHA-256 `34f4ee644087797cdad955dcc53f69a90479136b582a4359bf3eaa3673e04424`
  - `03_KK_PARTY_CARDS/PRIMARY_USER_SAVED/UNDERWEAR.png`: SHA-256 `f9770a7432fb32433b701b33b064786ad021e1ac7eccfd6f339211b0af4524d5`
  - `03_KK_PARTY_CARDS/PRIMARY_USER_SAVED/NUDE.png`: SHA-256 `b0298bbb3ebad6ce5e45e0d11437ed56891a106788ee6f6d8c510fbab22ec3de`
- Untouched original creator KK card; creator-attributed `うまべや（休業）`, Pixiv `https://www.pixiv.net/artworks/121515691`; source SHA `995e533c63f5db60a1d1dc6bd0ba6fc5a94b5704e1a7f94ac489e764d60e85a8`; creator redistribution **rights not verified**.
- Frozen 2D modeling image SHA `daea9a014a250a5e74dcbfc707bd0829992fc91d321d217fb690dad727d50413`; preserved unchanged.
- Older 7-coordinate normalized fallback cards (do **not** replace the newer user-saved primary cards), mod inventory, fidelity spec, unrigged original dress/accessory Blender and GLB drafts, and QA logs. Pre-HF Patch diagnostic geometry is *not* an accepted production mesh.

## Component A: selected full artworks + frozen 2D (separate, exact)

User selected two final cinematic illustration references; their original bytes are in the separate ZIP created in Gate 01's originating ChatGPT conversation (linked there):
`GATE01_LuminousValentine_Gate00_SelectedArt_and_Frozen2D_20261011.zip`

- Bytes: **2,678,055**
- SHA-256: `edb9a048ac978e2cdb629557130482cd605b86f1496dba9ad11e904eed7d8164`
- Artwork A (uploaded `816267.png`, actually JPEG 864×1536): SHA `21f8ce54a04e67398e563dd122c90e90fa4b8f1f6ded816e080e5b48dfa3e159`.
- Artwork B (uploaded `816268.png`, actually JPEG 864×1536): SHA `74e326e430349354330be79ab2e5be8d44e948ef3980064b676f9d446100d599`.
- Same frozen v3 2D modeling authority SHA `daea9a014a250a5e74dcbfc707bd0829992fc91d321d217fb690dad727d50413`.
- The selected splash A/B images show cinematic Blood Rose sword poses; they do **not** replace the original character's approved front-facing construction.

## Immediate Gate 00 relay

**Use both ZIP components** to make the transfer, not just the GitHub documentation. Component B is on the authorized Windows PC; Component A is attached to the originating Gate01 ChatGPT conversation and is not yet copied to Windows. Do not claim a single combined archive exists until exact originals are physically combined with matching hashes. **Neither artist source card nor user-uploaded private files were published to this public GitHub repository.**

Gate 11: Review authentic KK embedded data, rights, original card author/source, provenance, all SHA values, 4,524 installed ZIPMOD dependency registry and Gate 11 Registry v2 submission. Keep status PENDING, not ARTIFACT_READY.

Gate 12: In actual Koikatsu Party, load EACH of the 3 PRIMARY_USER_SAVED PNGs, select coordinate 0 and viewport CLOTHED, check no residual garments from accessory slots for nude/underwear. Their 9 clothing-slot IDs for coordinate 0 were verified:
- CLOTHED: `[2, 790, 12, 101, 1881, 102, 0, 2, 15]`
- UNDERWEAR: `[0, 0, 17, 177355771, 0, 0, 0, 0, 0]`
- NUDE: `[0, 0, 0, 0, 0, 0, 0, 0, 0]`.

**Other six coordinate presets on the user-created cards are not fully normalized.** Gate 12 must explicitly select coordinate 0 or create reviewed coordinate-normalized derived assets without altering the originals. Then Gate 12 does actual 3D export, skeleton/morph/texture/rig retention, UE 5.8.3 import and Samsung Galaxy S26 Ultra verification. Preserve all formats and measure performance. None of these acceptance tests is currently approved.

Gate 00/user: No explicit final 3D model-fidelity approval was granted for the latest post-HF-Patch model. Thus treat it as **ready for Gate 00 review and specialist intake**, not final release. Each subsequent skin/manifestation requires its own 2D freeze and three independent Koikatsu state cards.

## Persistent personal Library handoff
The exact artwork ZIP was also securely uploaded to the user's personal ChatGPT Library at `/Offline-Game/Gate01/Gate00-Handoffs/GATE01_LuminousValentine_Gate00_SelectedArt_and_Frozen2D_20261011.zip` (2,678,055 bytes, SHA-256 `edb9a048ac978e2cdb629557130482cd605b86f1496dba9ad11e904eed7d8164`). Gate00 can retrieve the file from the user's Library in another project conversation without requiring the user to download/reupload it manually. The three actual KK files remain secured in the offline Windows handoff ZIP, NOT public GitHub.
