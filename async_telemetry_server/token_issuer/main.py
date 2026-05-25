import os
import time
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from jose import jwt

app = FastAPI(title="ATS Token Issuer")

ISSUER = "https://auth.nexburg.internal"
SECRET = os.environ.get("JWT_SECRET", "dev_secret_change_in_prod")
TTL_S  = 3600

# Demo credentials — intentionally insecure, documented in SECURITY.md
CLIENTS: dict[str, str] = {
    "test_client":   "test_pass",
    "sensor_node_1": "sensor_pass",
}


class TokenRequest(BaseModel):
    client_id: str
    client_secret: str


class TokenResponse(BaseModel):
    access_token: str
    token_type: str = "bearer"
    expires_in: int = TTL_S


@app.post("/token", response_model=TokenResponse)
def issue_token(req: TokenRequest) -> TokenResponse:
    if CLIENTS.get(req.client_id) != req.client_secret:
        raise HTTPException(status_code=401, detail="invalid credentials")
    now = int(time.time())
    payload = {
        "iss": ISSUER,
        "sub": req.client_id,
        "iat": now,
        "exp": now + TTL_S,
    }
    token = jwt.encode(payload, SECRET, algorithm="HS256")
    return TokenResponse(access_token=token)
