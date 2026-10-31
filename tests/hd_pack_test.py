"""Synthetic, ROM-free HD pack creation tests.

Run: python3 -m unittest discover -s tests -p 'hd_pack_test.py' -v
Pillow required; no GBA ROM or game artwork used.
"""
import sys
from pathlib import Path
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from build_hd_pack import fingerprint, make_pack

try:
    from PIL import Image
except ImportError:
    Image = None


@unittest.skipIf(Image is None, "Pillow not installed")
class HdPackTests(unittest.TestCase):
    def test_hash_and_output(self):
        with tempfile.TemporaryDirectory() as temp:
            t = Path(temp)
            frame = Image.new("RGB", (240, 160), (40, 80, 120))
            frame.putpixel((6, 8), (111, 1, 19))
            frame.save(t/"native.png")
            art = Image.new("RGB", (64, 64), (50, 60, 220))
            art.save(t/"artist.png")
            data = make_pack(t/"native.png", t/"artist.png", t/"pack", 4, 6, 16, 16)
            self.assertEqual(data["version"], 1)
            self.assertEqual(data["regions"][0]["fingerprint"],
                             fingerprint(frame.tobytes(), 4, 6, 16, 16))
            self.assertTrue((t/"pack"/"panel.bmp").exists())
            self.assertEqual(Image.open(t/"pack"/"panel.bmp").size, (64, 64))
            self.assertFalse((t/"pack"/"native.png").exists())

    def test_reject_non_native_and_transparency(self):
        with tempfile.TemporaryDirectory() as temp:
            t = Path(temp)
            Image.new("RGB", (480, 320)).save(t/"native.png")
            Image.new("RGB", (32, 32)).save(t/"artist.png")
            with self.assertRaisesRegex(ValueError, "240x160"):
                make_pack(t/"native.png",t/"artist.png",t/"pack",0,0,16,16)
            Image.new("RGB", (240, 160)).save(t/"native.png")
            Image.new("RGBA", (64, 64), (255,0,0,120)).save(t/"artist.png")
            with self.assertRaisesRegex(ValueError, "fully opaque"):
                make_pack(t/"native.png",t/"artist.png",t/"pack",0,0,16,16)

    def test_bounds(self):
        with self.assertRaisesRegex(ValueError, "invalid native"):
            fingerprint(bytes(240*160*3), 220, 151, 40, 16)
        with self.assertRaisesRegex(ValueError, "frame must"):
            fingerprint(bytes(5), 0, 0, 2, 2)


if __name__=="__main__":
    unittest.main()
