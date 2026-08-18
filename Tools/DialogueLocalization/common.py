"""Shared deterministic XLSX/JSON exchange helpers for LostRunic localization."""

from __future__ import annotations

import hashlib
import json
import uuid
from datetime import datetime, timezone
from typing import Any, Iterable


IDENTITY_FIELDS = (
    "schemaVersion", "localizationTarget", "targetCulture", "scriptId", "stringKey",
    "sourceHash", "textTableId", "textTableKey", "locNamespace", "locKey", "msgCtxt", "msgId",
)

ENTRY_COLUMNS = (
    "ScriptId", "StringKey", "DisplayId", "SourceText", "SourceHash", "TextTableId", "TextTableKey",
    "LocNamespace", "LocKey", "MsgCtxt", "MsgId", "SpeakerId", "EntryType", "SourceLine", "Order",
    "ChoicePath", "ConditionalPath", "FormatArgs", "Translation", "TranslatorNote", "TranslatorContext",
)


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat()


def canonical_json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"), sort_keys=False)


def sha256_json(value: Any) -> str:
    return hashlib.sha256(canonical_json(value).encode("utf-8")).hexdigest()


def new_workbook_id() -> str:
    return str(uuid.uuid4())


def row_to_identity(row: dict[str, Any], schema_version: int, target: str, culture: str) -> dict[str, Any]:
    return {
        "schemaVersion": schema_version,
        "localizationTarget": target,
        "targetCulture": culture,
        "scriptId": row.get("ScriptId", row.get("scriptId", "")),
        "stringKey": row.get("StringKey", row.get("stringKey", "")),
        "sourceHash": row.get("SourceHash", row.get("sourceHash", "")),
        "textTableId": row.get("TextTableId", row.get("textTableId", "")),
        "textTableKey": row.get("TextTableKey", row.get("textTableKey", "")),
        "locNamespace": row.get("LocNamespace", row.get("locNamespace", "")),
        "locKey": row.get("LocKey", row.get("locKey", "")),
        "msgCtxt": row.get("MsgCtxt", row.get("msgCtxt", "")),
        "msgId": row.get("MsgId", row.get("msgId", "")),
    }


def speaker_to_identity(row: dict[str, Any], schema_version: int, target: str, culture: str) -> dict[str, Any]:
    return {
        "schemaVersion": schema_version,
        "localizationTarget": target,
        "targetCulture": culture,
        "speakerId": row.get("SpeakerId", row.get("speakerId", "")),
        "sourceHash": row.get("SourceHash", row.get("sourceHash", "")),
        "textTableId": row.get("TextTableId", row.get("textTableId", "")),
        "textTableKey": row.get("TextTableKey", row.get("textTableKey", "")),
        "locNamespace": row.get("LocNamespace", row.get("locNamespace", "")),
        "locKey": row.get("LocKey", row.get("locKey", "")),
        "msgCtxt": row.get("MsgCtxt", row.get("msgCtxt", "")),
        "msgId": row.get("MsgId", row.get("msgId", "")),
    }


def export_fingerprint(rows: Iterable[dict[str, Any]], schema_version: int, target: str, culture: str,
                      speakers: Iterable[dict[str, Any]] = ()) -> str:
    entry_identities = [row_to_identity(row, schema_version, target, culture) for row in rows]
    entry_identities.sort(key=lambda item: (item["scriptId"], item["stringKey"]))
    speaker_identities = [speaker_to_identity(row, schema_version, target, culture) for row in speakers]
    speaker_identities.sort(key=lambda item: item["speakerId"])
    return sha256_json({"entries": entry_identities, "speakers": speaker_identities})


def as_text(value: Any) -> str:
    return "" if value is None else str(value)


def parse_json_cell(value: Any) -> list[str]:
    if not value:
        return []
    if isinstance(value, list):
        return [as_text(item) for item in value]
    parsed = json.loads(as_text(value))
    if not isinstance(parsed, list):
        raise ValueError("FormatArgs must be a JSON array")
    return [as_text(item) for item in parsed]
