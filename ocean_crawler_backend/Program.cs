using Microsoft.EntityFrameworkCore;
using Newtonsoft.Json;
using Npgsql;
using static System.Runtime.InteropServices.JavaScript.JSType;
var builder = WebApplication.CreateBuilder(args);
// Add services to the container.
// Learn more about configuring OpenAPI at https://aka.ms/aspnet/openapi
var connectionString =
   builder.Configuration.GetValue<string>("DatabaseConnection");
builder.Services.AddDbContext<GpsContext>(options =>
    options.UseNpgsql(connectionString));

builder.Services.AddOpenApi();

var app = builder.Build();

// Configure the HTTP request pipeline.
if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
}

app.MapGet("/locationHistory", (GpsContext context) =>
{
    var result = new LocationInfo();
    var ideas = context.gps_history
            .OrderBy(b => b.Timestamp)
            .Take(100);
    ideas.ToList();
foreach (var data in ideas)
    {
        result.GpsHistory.Add(new GpsData(data.Timestamp, data.Latitude, data.Longitude));
    }
    result.currentLatitude = ideas?.Last().Latitude ?? 0.0;
    result.currentLongitude = ideas?.Last().Longitude ?? 0.0;

    //result.GpsHistory.Add(new GpsData(1700, -37.8905752, 147.9744201));
    //result.GpsHistory.Add(new GpsData(1700, -37.8931751, 147.977006));
    //result.GpsHistory.Add(new GpsData(1700, -37.895287, 147.9830989));
    //result.GpsHistory.Add(new GpsData(1700, -37.8960082, 147.9887009));
    //result.GpsHistory.Add(new GpsData(1700, -37.8959847, 147.9970343));
    //result.GpsHistory.Add(new GpsData(1700, -37.8979804, 148.0035925));
    //result.GpsHistory.Add(new GpsData(1700, -37.8968936, 148.0155115));
    //result.GpsHistory.Add(new GpsData(1700, -37.8965289,148.0191522));
    //result.GpsHistory.Add(new GpsData(1700, -37.8952437,148.0229046));
    //result.GpsHistory.Add(new GpsData(1700, -37.8942408,148.0273223));
    //result.GpsHistory.Add(new GpsData(1700, -37.8944553,148.0344886));
    //result.GpsHistory.Add(new GpsData(1700, -37.8943674,148.0393696));
    //result.GpsHistory.Add(new GpsData(1700, -37.8959815,148.0498477));
    //result.GpsHistory.Add(new GpsData(1700, -37.8979239,148.0600221));
    //result.GpsHistory.Add(new GpsData(1700, -37.9001827,148.0715247));
    //result.GpsHistory.Add(new GpsData(1700, -37.9015747,148.0918328));
    //result.GpsHistory.Add(new GpsData(1700, -37.8962271,148.1317267));


    var jsonString = JsonConvert.SerializeObject(result);
    return jsonString;
})
.WithName("locationHistory");

app.Run();

class GpsData {
    public long timestamp;
    public double latitude;
    public double longitude;
    public GpsData(long timestamp, double longitude, double latitude)
    {
        this.timestamp = timestamp;
        this.latitude = latitude;
        this.longitude = longitude;
    }
}
class LocationInfo()
{
    public double currentLatitude;
    public double currentLongitude;
    public List<GpsData> GpsHistory = new List<GpsData>();

}