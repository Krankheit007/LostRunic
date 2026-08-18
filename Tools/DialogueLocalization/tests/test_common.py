import json
import gc
import sys
import tempfile
import unittest
from pathlib import Path

from export_xlsx import main as export_xlsx_main
from common import export_fingerprint
from import_xlsx import main as import_xlsx_main


def run_script(main, arguments):
    previous_argv = sys.argv
    try:
        sys.argv = ["dialogue_localization_test", *arguments]
        return main()
    finally:
        sys.argv = previous_argv


class CommonTests(unittest.TestCase):
    def test_export_fingerprint_ignores_translation(self):
        row = {"scriptId": "Home.Butler.Introduction", "stringKey": "@001a@", "sourceHash": "abc"}
        first = export_fingerprint([row | {"translation": "甲"}], 1, "LostRunic", "en")
        second = export_fingerprint([row | {"translation": "Butler"}], 1, "LostRunic", "en")
        self.assertEqual(first, second)

    def test_workbook_export_import_round_trip_preserves_identity(self):
        data = {
            "schemaVersion": 1,
            "localizationTarget": "LostRunic",
            "manifestHash": "manifest-hash",
            "poRevision": "123",
            "rows": [{
                "scriptId": "Home.Butler.Introduction", "stringKey": "@001a@", "displayId": "line",
                "sourceText": "冷。", "sourceHash": "source-hash", "textTableId": "ST_Dialogue",
                "textTableKey": "001a", "locNamespace": "LostRunic", "locKey": "001a",
                "msgCtxt": "NS", "msgId": "ID", "speakerId": "Butler", "entryType": "SpeakerLine",
                "sourceLine": 1, "order": 0, "choicePath": "", "conditionalPath": "",
                "formatArgs": [], "translation": "Cold.", "translatorNote": "Keep punctuation.",
                "translatorContext": "Intro",
            }],
            "speakers": [{
                "speakerId": "Butler", "displayName": "老管家", "sourceText": "老管家",
                "sourceHash": "speaker-hash", "textTableId": "ST_DialogueSpeakers",
                "textTableKey": "Butler", "locNamespace": "LostRunicSpeakers", "locKey": "Butler",
                "msgCtxt": "SpeakerNS", "msgId": "SpeakerID", "translation": "Butler",
                "translatorNote": "",
            }],
        }
        with tempfile.TemporaryDirectory() as directory:
            input_path = Path(directory) / "rows.json"
            workbook_path = Path(directory) / "Dialogue_en.xlsx"
            import_path = Path(directory) / "import.json"
            input_path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
            self.assertEqual(run_script(export_xlsx_main, [
                "--input", str(input_path), "--output", str(workbook_path), "--culture", "en",
            ]), 0)
            self.assertTrue(workbook_path.exists())
            workbook = __import__("openpyxl").load_workbook(workbook_path, read_only=False, data_only=True)
            self.assertEqual(workbook.sheetnames, ["Meta", "Entries", "Speakers"])
            self.assertEqual(workbook["Entries"].cell(row=2, column=19).value, "Cold.")
            self.assertEqual(workbook["Speakers"].cell(row=2, column=11).value, "Butler")
            workbook.close()
            del workbook
            gc.collect()
            self.assertEqual(run_script(import_xlsx_main, [
                "--input", str(workbook_path), "--output", str(import_path), "--culture", "en",
            ]), 0)
            imported = json.loads(import_path.read_text(encoding="utf-8"))
            self.assertEqual(imported["rows"][0]["msgId"], "ID")
            self.assertEqual(imported["speakers"][0]["msgId"], "SpeakerID")


if __name__ == "__main__":
    unittest.main()
