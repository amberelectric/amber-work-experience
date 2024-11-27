echo "Enter your API key: "
read -r API_KEY
echo "Enter your Site ID: "
read -r SITE_ID
mkdir v1
mkdir v1/sites
mkdir v1/sites/$SITE_ID
mkdir v1/sites/$SITE_ID/prices
curl -H "Authorization: Bearer $API_KEY" https://api.amber.com.au/v1/sites/$SITE_ID/prices/current > v1/sites/$SITE_ID/prices/current
python3 -m http.server
