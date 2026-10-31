"""ROM-free tests: metadata-only GBA sprite identity analyzer."""
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from analyze_hd_obj_capture import parse_capture


def sample(frame_no, x, y, *, tile="0123456789abcdef", palette="fedcba9876543210"):
    return {
        "frame": frame_no,
        "dispcnt": 4096,
        "bg": [[i,0,0,0,"1111111111111111"] for i in range(4)],
        "objs": [[5,0,0,0,x,y,16,32,12,1,0,0,tile,palette]],
    }


def capture(*frames):
    header={"format":"gba-hd-attribution-v1",
            "note":"hashes and OAM/BG metadata only; no graphics data"}
    return "\n".join(json.dumps(x) for x in (header, *frames)) + "\n"


class AnalyzeHdCaptureTest(unittest.TestCase):
    def test_stable_identity_tracks_movement(self):
        result=parse_capture(capture(
            sample(0,10,30),sample(30,12,30),sample(60,14,30)))
        self.assertEqual(result["observed_frames"],3)
        self.assertEqual(len(result["obj_candidates"]),1)
        obj=result["obj_candidates"][0]
        self.assertEqual(obj["candidate_id"],"obj_001")
        self.assertEqual(obj["tile_hash"],"0123456789abcdef")
        self.assertEqual(obj["width"],16)
        self.assertTrue(obj["movement_observed"])
        self.assertEqual(obj["different_positions"],3)
        self.assertEqual(len(result["bg_candidates"]),4)
        self.assertNotIn("Goku", json.dumps(result))

    def test_palette_changes_distinguish_identities(self):
        a=sample(0,10,30)
        b=sample(30,10,30,palette="aaaaaaaaaaaaaaaa")
        result=parse_capture(capture(a,b))
        self.assertEqual(len(result["obj_candidates"]),2)
        self.assertFalse(result["obj_candidates"][0]["movement_observed"])

    def test_reject_bad_frames_and_records(self):
        with self.assertRaisesRegex(ValueError,"duplicate"):
            parse_capture(capture(sample(0,0,0),sample(0,1,0)))
        frame=sample(0,0,0)
        frame["objs"][0][12]="unsafe"
        with self.assertRaisesRegex(ValueError,"hash"):
            parse_capture(capture(frame))
        frame=sample(0,0,0)
        frame["objs"][0][0]=128
        with self.assertRaisesRegex(ValueError,"hardware ranges"):
            parse_capture(capture(frame))
        frame=sample(0,0,0)
        frame["objs"].append(frame["objs"][0])
        with self.assertRaisesRegex(ValueError,"duplicate OAM"):
            parse_capture(capture(frame))
        with self.assertRaisesRegex(ValueError,"header"):
            parse_capture(json.dumps({"format":"invalid"}) + "\\n" + json.dumps(sample(1,1,1)) + "\\n")

    def test_limit_and_non_frame(self):
        with self.assertRaisesRegex(ValueError,"1..120"):
            parse_capture(capture(*[sample(i,1,1) for i in range(121)]))
        with self.assertRaisesRegex(ValueError,"requires"):
            parse_capture(json.dumps({"format":"gba-hd-attribution-v1"}))


if __name__ == "__main__":
    unittest.main()
