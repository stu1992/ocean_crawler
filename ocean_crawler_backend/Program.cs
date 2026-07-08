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
builder.Services.AddDbContext<DiagnosticsContext>(options =>
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
    var jsonString = JsonConvert.SerializeObject(result);
    return jsonString;
})
.WithName("locationHistory");

app.MapPost("/addHistory", (List<GpsData> locations, GpsContext context) =>
{
    //TODO authenticate via uuid to get CrawlerId
    foreach( var point in locations)
    {
        var entity = new GpsHistoryEntity
        {
            CrawlerId = 1,
            Timestamp = point.timestamp,
            Longitude = point.longitude,
            Latitude = point.latitude
        };

        context.gps_history.Add(entity);

    }
        context.SaveChangesAsync();

})
.WithName("addHistory");

app.MapPost("/addDiagnostics", (DiagnosticsDto diagnostics, DiagnosticsContext context) =>
{
    //TODO authenticate via uuid to get CrawlerId
    var entity = new DiagnosticsHistoryEntity
    {
        CrawlerId = 1,
        Timestamp = diagnostics.timestamp,
        averageSatelliteCount = diagnostics.averageSatelliteCount,
        averageTryCount = diagnostics.averageTryCount,
        averageWakeupPerLock = diagnostics.averageWakeupPerLock
    };

    context.diagnostics_history.Add(entity);
    context.SaveChangesAsync();
})
.WithName("addDiagnostics");

app.Run();

public record HistoryDto(string uuid, List<GpsData> locations);

public record GpsData(long timestamp, double longitude, double latitude);
class LocationInfo()
{
    public double currentLatitude;
    public double currentLongitude;
    public List<GpsData> GpsHistory = new List<GpsData>();

}

public record DiagnosticsDto(long timestamp, double averageTryCount, double averageSatelliteCount, double averageWakeupPerLock);