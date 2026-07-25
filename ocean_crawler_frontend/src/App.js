
import './App.css';
import {Map} from './Map';
import { useState, useEffect } from 'react';
import 'leaflet/dist/leaflet.css';
function App() {
    const [gpsData, setgpsData] = useState(null);

    useEffect(() => {
      const startFetching = async () => {
      const response = await fetch(`/api/locationHistory/1`);
      const json = await response.json();
      setgpsData(json);
    }
    startFetching();
  }, []);

  return (
        <>
    <h1>Ocean Crawler</h1>

    <div className="App">
      {gpsData !== undefined && gpsData !== null && <Map gpsData={gpsData}/> }
    </div>
    </>
  );
}

export default App;
