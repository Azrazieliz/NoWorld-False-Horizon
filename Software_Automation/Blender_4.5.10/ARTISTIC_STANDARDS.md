# Artistic Standards and Verification Priorities

## Identity and style
The target game style is intentional, premium stylized/anime construction: clear contour hierarchy, coherent anatomical proportions, distinct silhouettes and faces, calibrated highlights, deliberate materials, and clean 2D imagery instead of pseudo-3D skin aesthetics. Never treat a quick Workbench preview or primitive wing mesh as artistic acceptance.

## Character / creature QC (not yet executed)
- Measure facial landmarks, hair silhouette, body ratios, bone references and garment fit before and after each conversion stage.
- Compare frames in consistent orthographic and perspective camera/lens/lighting. Use overlaid silhouettes, key colors, UV texture resolution and shape-key interpolation measurements.
- Character-specific face blendshapes and motion are preferred over generic template deformation.
- Validate foot plant contact windows, speed mismatch, knee/ankle popping, root-trajectory velocity continuity, toe-floor penetration, turn alignment, joint bounds and character mass/acceleration.
- Mounts and nonhumanoids require anatomy-specific skeletal layouts and appropriate rider anchors.

## 2D and hybrid QC (not yet executed)
- Rendered frames must preserve hand-drawn 2D anime principles: refined contours, purposeful limited animation/holds, high-quality expressive timing, sharp cel layers and compositional continuity. Interpolation alone is not adequate.
- Grease Pencil stroke density, fill ordering, line art occlusion, aliasing, render passes and layered compositing require dedicated tests.
- Compare Blender Grease Pencil, Krita, OpenToonz and Synfig where those tools offer a superior actual automated capability. Tools are candidates, not installed claims.

## Materials / texture QC (engineering node test only)
- Local and world gradients, asymmetry and damage masks must remain independently editable at source. Unreal material instance controls must be separately built and judged.
- Reject visible seams, accidental gradient discontinuity, wrong color-space transforms, missing alpha, poor hair transparency, baked toon artifacts and blurred eye details.
- Track texture dimensions, color space, UV overlap (except intended mirror), tangent-space orientation, MikkTSpace normal parity, atlas loss and mobile shader performance.

## Performance
Budget geometry, texture memory, draw calls, skeletal bones, materials and animation cost for Android-first operation. Draft previews should be cheap; final quality must be evaluated separately on the target pipeline. No numerical asset budget is approved at this stage.

## Acceptance
A domain cannot become production-ready from a successful API call, code compilation, OBJ/FBX presence or uninspected render. Require domain-specific artistic review and independent technical tests.
