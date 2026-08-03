import React from 'react';
import { MapContainer , TileLayer, Polyline, CircleMarker, Popup, useMap  } from 'react-leaflet'

function ChangeView({ center, zoom }) {
  const map = useMap();
  map.setView(center, zoom);
  return null;
}


export function Map({gpsData}) {

let lastKnownTimestamp = new Date(gpsData['GpsHistory'].at(-1)['timestamp'] * 1000);

const chunkArray = (arr, size) => {
  return arr.reduce((acc, _, i) => {
    if (i % size === 0) {
      acc.push(arr.slice(i-1, i + size));
    }
    return acc;
  });
};
let chunks = chunkArray(gpsData['GpsHistory'].map((x) => [x['longitude'], x['latitude']]).reverse(), 2);
//chunks.shift();

return(
  <MapContainer center={[gpsData['currentLatitude'],gpsData['currentLongitude']]} zoom={9} scrollWheelZoom={true} tap={false}>
    <ChangeView center={[gpsData['currentLatitude'],gpsData['currentLongitude']]} zoom={9} /> 
    <TileLayer
      attribution='&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
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