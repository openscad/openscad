
#!/usr/bin/env bash
# Semi-automated Developer ID Application renewal helper for OpenSCAD

set -e

# Persistent directory so private keys are never lost if a step fails
KEYDIR="$HOME/.openscad_signing"
mkdir -p "$KEYDIR"
chmod 700 "$KEYDIR"

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
KEY_FILE="$KEYDIR/developer_id_${TIMESTAMP}.key"
CSR_FILE="$KEYDIR/developer_id_${TIMESTAMP}.csr"
P12_FILE="$KEYDIR/OpenSCAD_DeveloperID_${TIMESTAMP}.p12"
G2_CA_FILE="$KEYDIR/DeveloperIDG2CA.cer"

COMMON_NAME="Developer ID Application: Marius Kintel (28U8KJ6T2P)"

clean_path() {
    local p="$1"
    # Trim leading and trailing whitespace
    p="${p#"${p%%[![:space:]]*}"}"
    p="${p%"${p##*[![:space:]]}"}"
    # Strip surrounding single or double quotes
    if [[ "$p" =~ ^\"(.*)\"$ ]] || [[ "$p" =~ ^\'(.*)\'$ ]]; then
        p="${BASH_REMATCH[1]}"
    fi
    # Expand ~ to $HOME
    p="${p/#\~/$HOME}"
    echo "$p"
}

echo "==> Step 1: Generating 2048-bit RSA Private Key and CSR..."
openssl req -new -newkey rsa:2048 -nodes \
    -keyout "$KEY_FILE" \
    -out "$CSR_FILE" \
    -subj "/CN=${COMMON_NAME}/OU=28U8KJ6T2P/O=Marius Kintel/C=CA"

chmod 600 "$KEY_FILE"

cp "$CSR_FILE" ~/Desktop/OpenSCAD_DeveloperID.csr
echo "    Private key saved safely to: $KEY_FILE"
echo "    CSR copied to: ~/Desktop/OpenSCAD_DeveloperID.csr"
echo ""
echo "=========================================================================="
echo "==> Step 2: Apple Developer Portal (Manual)"
echo "    1. Open: https://developer.apple.com/account/resources/certificates/add"
echo "    2. Select 'Developer ID Application' -> Continue"
echo "    3. Select 'G2 Sub-CA' when prompted for the intermediary"
echo "    4. Upload: ~/Desktop/OpenSCAD_DeveloperID.csr"
echo "    5. Download the issued .cer file"
echo "=========================================================================="
echo ""

DEFAULT_CER="$HOME/Downloads/developerID_application.cer"

while true; do
    read -r -p "Enter path to downloaded .cer file [Default: ~/Downloads/developerID_application.cer]: " USER_INPUT
    if [ -z "$USER_INPUT" ]; then
        RESOLVED_PATH="$DEFAULT_CER"
    else
        RESOLVED_PATH=$(clean_path "$USER_INPUT")
    fi

    if [ -f "$RESOLVED_PATH" ]; then
        echo "Found certificate: $RESOLVED_PATH"
        break
    else
        echo "Error: File not found at '$RESOLVED_PATH'. Please check the path and try again."
    fi
done

echo ""
echo "==> Step 3: Fetching Apple Developer ID G2 Intermediate CA..."
if [ ! -f "$G2_CA_FILE" ]; then
    curl -s -o "$G2_CA_FILE" https://www.apple.com/certificateauthority/DeveloperIDG2CA.cer
fi

# Verify public key matches private key
KEY_MOD=$(openssl rsa -in "$KEY_FILE" -noout -modulus)
CER_MOD=$(openssl x509 -inform der -in "$RESOLVED_PATH" -noout -modulus)

if [ "$KEY_MOD" != "$CER_MOD" ]; then
    echo "ERROR: The certificate in '$RESOLVED_PATH' does NOT match the private key generated in this run!"
    echo "This happens if you uploaded an older CSR to Apple. Please create a new certificate on Apple Developer portal using ~/Desktop/OpenSCAD_DeveloperID.csr and download the new .cer file."
    exit 1
fi

TMP_DIR=$(mktemp -d -t "p12-build-XXXXXX")
trap 'rm -rf "$TMP_DIR"' EXIT

openssl x509 -inform der -in "$RESOLVED_PATH" -out "$TMP_DIR/cert.pem"
openssl x509 -inform der -in "$G2_CA_FILE" -out "$TMP_DIR/g2.pem"

echo ""
read -s -r -p "Enter password to encrypt the new .p12 (APPLE_CODE_SIGNING_PASSWORD): " P12_PASSWORD
echo ""

echo "==> Step 4: Bundling into .p12 with full trust chain..."
openssl pkcs12 -export \
    -inkey "$KEY_FILE" \
    -in "$TMP_DIR/cert.pem" \
    -certfile "$TMP_DIR/g2.pem" \
    -out "$P12_FILE" \
    -passout pass:"$P12_PASSWORD"

BASE64_KEY=$(base64 -i "$P12_FILE")

echo ""
echo "==> SUCCESS! New .p12 certificate generated:"
echo "    $P12_FILE"
echo ""
echo "Base64 string copied to clipboard (via pbcopy)."
echo "$BASE64_KEY" | pbcopy
echo ""
echo "If you ever need to re-copy it to your clipboard, run:"
echo "    base64 -i \"$P12_FILE\" | pbcopy"
echo ""

if [ -n "$CIRCLECI_TOKEN" ]; then
    echo "Updating CircleCI environment variables..."
    curl -s -X POST \
      -H "Circle-Token: $CIRCLECI_TOKEN" \
      -H "Content-Type: application/json" \
      -d "{\"name\":\"APPLE_CODE_SIGNING_KEY\",\"value\":\"$BASE64_KEY\"}" \
      "https://circleci.com/api/v2/project/gh/openscad/openscad/envvar" > /dev/null

    curl -s -X POST \
      -H "Circle-Token: $CIRCLECI_TOKEN" \
      -H "Content-Type: application/json" \
      -d "{\"name\":\"APPLE_CODE_SIGNING_PASSWORD\",\"value\":\"$P12_PASSWORD\"}" \
      "https://circleci.com/api/v2/project/gh/openscad/openscad/envvar" > /dev/null
    echo "CircleCI environment variables updated successfully!"
else
    echo "Paste the clipboard into CircleCI Organization Context 'secret-context':"
    echo "    https://app.circleci.com/settings/organization/github/openscad/contexts"
    echo ""
    echo "Click 'secret-context' and update the Environment Variables:"
    echo "  - APPLE_CODE_SIGNING_KEY: <pasted from clipboard>"
    echo "  - APPLE_CODE_SIGNING_PASSWORD: <your password>"
    echo ""
    echo "To re-copy the key to your clipboard at any time:"
    echo "    base64 -i \"$P12_FILE\" | pbcopy"
fi

