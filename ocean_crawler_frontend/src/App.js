
import './App.css';
import {Map} from './Map';
import { useState, useEffect } from 'react';
import 'leaflet/dist/leaflet.css';
function App() {
    const [gpsData, setgpsData] = useState(null);
    const [crawlerSelect, setCrawlerSelect] = useState(null)

  // 2. Manage the selected value state
  const [selectedValue, setSelectedValue] = useState(1);

  // 3. Handle selection changes
  const handleChange = (event) => {
    setSelectedValue(event.target.value);
  };

    useEffect(() => {
      const startFetching = async () => {
      const crawlerRequest = await fetch(`api/crawlers`);
      const obj = await crawlerRequest.json();
      let crawlers = [];
      for (const key of Object.keys(obj)) {
        crawlers.push({ value: obj[key], label: key })
      }
      setCrawlerSelect(crawlers);
      const response = await fetch('api/locationHistory/'+ selectedValue);
      const json = await response.json();
      setgpsData(json);
    }
    startFetching();
  }, [selectedValue]);

  return (
        <>
    <h1>Ocean Crawler</h1>
    { crawlerSelect != null &&
      <select 
        id="fruit-select" 
        value={crawlerSelect[0].value} 
        onChange={handleChange}
      >
        <option value="1" disabled>select a crawler</option>
        {crawlerSelect.map((option) => (
          <option key={option.value} value={option.value}>
            {option.label}
          </option>
        ))}
      </select>
    }
    <div className="App">
      {gpsData !== undefined && gpsData !== null && <Map gpsData={gpsData}/> }
    </div>
    </>
  );
}

export default App;
