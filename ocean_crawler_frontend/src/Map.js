import React from 'react';
import { MapContainer, TileLayer, useMap, Marker, Polyline, CircleMarker, Popup } from 'react-leaflet'
export function Map() {
const position = [-38.1415665,145.1510630]

const multiPolyline = [
  [
    [-38.1415665,145.1510630],
    [-38.1515700,145.20106800],
    [-38.1815665,145.3010630],
    [-38.1815665,145.3010630],
    [-35.1515700,140.20106800],
    [-32.1815665,138.3010630],
  ]
]
const limeOptions = { color: '#000000' }
const redOptions = { color: '#FF0000' }
return(
    
  <MapContainer center={position} zoom={13} scrollWheelZoom={true}>
    <TileLayer
      attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
      url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
    />
<Polyline pathOptions={limeOptions} positions={multiPolyline} />
<CircleMarker center={[-38.1415665,145.1510630]} pathOptions={redOptions} radius={20}>
      <Popup>Popup in CircleMarker</Popup>
    </CircleMarker>
  </MapContainer>
);


}