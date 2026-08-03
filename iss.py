import time
import requests

location_url = "http://192.168.0.150/api/addHistory"
iss_url = "https://www.astroviewer.net/iss/ws/orbit.php?sat=25544"

while True:
 response = requests.get(iss_url)
 print(response.json()['tRef'])
 print(response.json()['orbitData'][0]['lt'])
 print(response.json()['orbitData'][0]['ln'])
 payload = {"uuid":"3de4381b-b352-4652-8d81-420bf049641d", "readings":[{"timestamp":response.json()['tRef'],"latitude":response.json()['orbitData'][0]['lt'], "longitude":response.json()['orbitData'][0]['ln']}]}
 response = requests.post(location_url, json=payload)
 print(response.status_code)
 time.sleep(120)
