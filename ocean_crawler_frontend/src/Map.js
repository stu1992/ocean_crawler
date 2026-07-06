import React from 'react';
import { MapContainer, TileLayer, useMap, Marker, Polyline, CircleMarker, Popup } from 'react-leaflet'
export function Map({gpsData}) {
    console.log(gpsData);
const position = [-38.1415665,145.1510630]

let multiPolyline = [[]] 
multiPolyline[0]=gpsData['GpsHistory'].map((x) => [x['longitude'], x['latitude']]);
var lastKnownTimestamp = new Date(gpsData['GpsHistory'][gpsData['GpsHistory'].length-1]['timestamp'] * 1000);

const limeOptions = { color: '#000000' }
const redOptions = { color: '#FF0000' }
return(
  <MapContainer center={[gpsData['currentLatitude'],gpsData['currentLongitude']]} zoom={13} scrollWheelZoom={true}>
    <TileLayer
      attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
      url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
    />
<Polyline pathOptions={limeOptions} positions={multiPolyline} />
<CircleMarker center={[gpsData['currentLatitude'],gpsData['currentLongitude']]} pathOptions={redOptions} radius={20}>
      <Popup>last known whereabouts at {lastKnownTimestamp.toLocaleTimeString()} on {lastKnownTimestamp.toLocaleDateString()} </Popup>
    </CircleMarker>
  </MapContainer>
);


}