import React from 'react';
import { MapContainer, TileLayer, useMap, Marker, Polyline, CircleMarker, Popup } from 'react-leaflet'
export function Map({gpsData}) {

let multiPolyline = [[]] 
multiPolyline[0]=gpsData['GpsHistory'].map((x) => [x['longitude'], x['latitude']]);
var lastKnownTimestamp = new Date(gpsData['GpsHistory'][0]['timestamp'] * 1000);

const chunkArray = (arr, size) => {
  return arr.reduce((acc, _, i) => {
    if (i % size === 0) {
      acc.push(arr.slice(i-1, i + size));
    }
    return acc;
  }, []);
};
let chunks = chunkArray(multiPolyline[0], 2);
chunks.shift();
return(
  <MapContainer center={[gpsData['currentLatitude'],gpsData['currentLongitude']]} zoom={13} scrollWheelZoom={true}>
    <TileLayer
      attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
      url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
    />
    {
      chunks.map((chunk, index) => ( 
        <Polyline key={index} pathOptions={{ color: '#000000', opacity: 1.0 -(index * (1 / chunks.length ))  }} positions={chunk}/>
      ))
    }
<CircleMarker center={[gpsData['currentLatitude'],gpsData['currentLongitude']]} pathOptions={{ color: '#FF0000' }} radius={20}>
      <Popup>last known whereabouts at {lastKnownTimestamp.toLocaleTimeString()} on {lastKnownTimestamp.toLocaleDateString()} </Popup>
    </CircleMarker>
  </MapContainer>
);


}