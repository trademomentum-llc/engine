#!/usr/bin/env python3
"""Ingest NNOS spec chain into provenance corpus."""
import sys
import os
import hashlib
from pathlib import Path
from datetime import datetime

os.environ["LOKY_MAX_CPU_COUNT"] = "4"

from qdrant_client import QdrantClient
from qdrant_client.models import PointStruct
from sentence_transformers import SentenceTransformer

QDRANT_URL = "http://127.0.0.1:6333"
COLLECTION = "provenance_corpus"
CHUNK_SIZE = 2000
CHUNK_OVERLAP = 200

client = QdrantClient(url=QDRANT_URL)
model = SentenceTransformer("all-MiniLM-L6-v2")

spec_file = Path(__file__).parent / "NNOS-FULL-SPEC-CHAIN.md"
content = spec_file.read_text()

# Chunk it
chunks = []
start = 0
while start < len(content):
    end = start + CHUNK_SIZE
    chunk = content[start:end]
    if chunk.strip():
        chunks.append(chunk)
    start = end - CHUNK_OVERLAP

points = []
for i, chunk in enumerate(chunks):
    doc_id = hashlib.sha256(
        f"nnos-spec-chain::{i}::{chunk[:80]}".encode()
    ).hexdigest()[:16]
    point_id = int(hashlib.sha256(doc_id.encode()).hexdigest()[:8], 16)
    embedding = model.encode(chunk).tolist()
    points.append(PointStruct(
        id=point_id,
        vector=embedding,
        payload={
            "doc_id": doc_id,
            "content": chunk[:8000],
            "author": "nexus1",
            "timestamp": datetime.now().isoformat(),
            "context_tags": [
                "nnos", "spec-chain", "validation-layer", "morph-engine",
                "quantum-morph", "neurodivergent-patterns", "jasterish",
                "behavioral-health", "dimensional-intelligence",
            ],
            "source_file": str(spec_file),
            "file_type": "md",
            "chunk_index": i,
            "total_chunks": len(chunks),
            "source": "nnos_spec_chain",
        },
    ))

if points:
    client.upsert(collection_name=COLLECTION, points=points)

info = client.get_collection(COLLECTION)
print(f"Ingested {len(points)} NNOS spec chain chunks")
print(f"Corpus total: {info.points_count} points")
