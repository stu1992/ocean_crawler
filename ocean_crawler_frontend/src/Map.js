import React from 'react';
import { MapContainer , TileLayer, LayersControl, Polyline, CircleMarker, Popup, useMap  } from 'react-leaflet'

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
let chunks = chunkArray(gpsData['GpsHistory'].map((x) => [x['longitude'], x['latitude']]), 2);
// we have an incomplete tail
chunks.shift();
chunks.shift();
// we want to reverse transparancy tweening
chunks = chunks.reverse();
chunks = chunks.filter(x => (x[1][1] * x[2][1] >0 && x[1][1] * x[2][1] <25000) || (x[1][1] * x[2][1] <0 && x[1][1] * x[2][1] >-50));

console.log(chunks);
return(
  <MapContainer center={[gpsData['currentLatitude'],gpsData['currentLongitude']]} zoom={9} scrollWheelZoom={true} tap={false}>
    <ChangeView center={[gpsData['currentLatitude'],gpsData['currentLongitude']]} zoom={9} /> 

        <LayersControl position="topright">
          <LayersControl.BaseLayer checked name="Open maps">
          <TileLayer
            attribution='&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
            url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
         />
        <LayersControl.BaseLayer name="Google Satellite">
          <TileLayer
            url="https://{s}.google.com/vt/lyrs=s&x={x}&y={y}&z={z}"
            maxZoom={20}
            subdomains={['mt0', 'mt1', 'mt2', 'mt3']}
            attribution="&copy; Google Maps"
          />
        </LayersControl.BaseLayer>
       <LayersControl.BaseLayer name="Google Terrain">
          <TileLayer
            url="https://{s}.google.com/vt/lyrs=p&x,h&x={x}&y={y}&z={z}"
            maxZoom={20}
            subdomains={['mt0', 'mt1', 'mt2', 'mt3']}
            attribution="&copy; Google Maps"
          />
        </LayersControl.BaseLayer>

    </LayersControl.BaseLayer>
    </LayersControl>

    {
      // the 100 calculation is a lngitude, either 170+ degrees or -170- degrees, i want to filter out the extreams going between that limit to the other side
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