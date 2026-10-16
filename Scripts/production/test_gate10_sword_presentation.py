#!/usr/bin/env python3
"""Source-regression guard: visible basic attack is a BLADE emblem, not a gun."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
STYLE = ROOT / "Source/OfflineGame/Private/World/OGGamePresentationStyle.cpp"
WORLD = ROOT / "Source/OfflineGame/Private/World/OGStartingRegionPresentation.cpp"


class SwordPresentationTests(unittest.TestCase):
    def test_primary_attack_draws_blade_not_firearm(self):
        code = STYLE.read_text(encoding="utf-8")
        start = code.index("void AOGWorldPresentationHud::DrawGameIcon")
        end = code.index("void AOGWorldPresentationHud::DrawGameButton", start)
        icon = code[start:end]
        match = re.search(r'if\(Name\.Contains\(TEXT\("attack"\)\).*?\}\s*else if', icon, re.S)
        self.assertIsNotNone(match, "basic attack icon branch missing")
        sword_branch = match.group()
        self.assertIn('Name.Contains(TEXT("sword"))', sword_branch)
        self.assertIn("DrawGamePolygon", sword_branch)
        self.assertIn("grip", sword_branch)
        self.assertIn("guard", sword_branch)
        self.assertNotRegex(sword_branch, r'(?i)\bprojectile\b|\bshoot\b|\brifle\b|\bpistol\b')

    def test_world_attack_reuses_real_basic_action(self):
        code = WORLD.read_text(encoding="utf-8")
        self.assertIn('DrawWorldActionButton(TEXT("Attack"), TEXT("OG.World.Attack")', code)
        self.assertIn('FName(TEXT("foundation.primary_attack"))', code)
        # The icon is the visual language, not a fake sword mesh granted to a
        # character that has no approved equipped sword content yet.
        self.assertNotIn('SpawnActor<AGun', code)
        self.assertNotIn('SpawnActor<ARifle', code)


if __name__ == "__main__":
    unittest.main()
