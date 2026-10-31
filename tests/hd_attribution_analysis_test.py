"""Synthetic attribution interpretation. No ROM, screenshots or game assets."""
import sys
from pathlib import Path
import unittest
import json
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from analyze_hd_attribution import analyze

class AttributionAnalysisTests(unittest.TestCase):
    @staticmethod
    def lines():
        result = [json.dumps({"format":"gba-hd-attribution-v1"})]
        for n in range(6):
            bg = [[i,0,0,0,"0000000000000001"] for i in range(4)]
            bg[1] = [1,0,n*3,n,"0000000000000002"]
            sprites = [
                [4,0,0,0,50+n*6,90+n,16,16,7,2,4,0,
                 f"{(n//2)+1:016x}","0000000000000088"],
                [17,0,0,0,12,10,8,8,1,0,0,0,
                 "0000000000000001","0000000000000001"]
            ]
            result.append(json.dumps({"frame":n*30,"dispcnt":0x1040,
                                     "bg":bg,"objs":sprites}))
        return result

    def test_tracks_and_scrolling_bg(self):
        result = analyze(self.lines())
        self.assertEqual(result["captured_frames"],6)
        self.assertEqual(result["object_tracks_seen"],2)
        self.assertGreaterEqual(len(result["motion_candidates"]),1)
        candidate = result["motion_candidates"][0]
        self.assertEqual(candidate["oam_slot"],4)
        self.assertEqual(candidate["palette_bank"],4)
        self.assertEqual(candidate["tile_patterns"],3)
        self.assertEqual(candidate["x_span"],[50,80])
        self.assertEqual(result["bg_layers"][1]["scroll_x_span"],[0,15])
        self.assertIn("do NOT prove",result["caution"])

    def test_reject_bad_capture(self):
        for data in [[],["{}"],self.lines()[:1]]:
            with self.assertRaises(ValueError):
                analyze(data)
        invalid=self.lines()
        invalid[-1] = invalid[1] # repeated/out-of-order frame
        with self.assertRaisesRegex(ValueError,"increase"):
            analyze(invalid)
        invalid=self.lines()
        frame=json.loads(invalid[1])
        frame["objs"][0][12] = "nothex0000000000"
        invalid[1]=json.dumps(frame)
        with self.assertRaises(ValueError):
            analyze(invalid)

if __name__=="__main__":
    unittest.main()
