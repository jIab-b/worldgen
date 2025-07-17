import hashlib

def hash_seed(seed: int, salt: str) -> int:
    """
    Creates a deterministic integer hash from a seed and a string salt.
    """
    hasher = hashlib.sha256()
    hasher.update(str(seed).encode())
    hasher.update(salt.encode())
    return int(hasher.hexdigest(), 16) 