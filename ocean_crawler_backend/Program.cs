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
builder.Services.AddDbContext<CrawlerContext>(options =>
    options.UseNpgsql(connectionString));

builder.Services.AddOpenApi();

var app = builder.Build();

// Configure the HTTP request pipeline.
if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
}

app.MapGet("/crawlers", (CrawlerContext crawlersContext) =>
{
    var result = new Dictionary<string, int>();
    var crawlers = crawlersContext.crawlers.ToList();
    foreach (var crawler in crawlers)
    {
        if(crawler.name != null) {
            result[crawler.name] = crawler.id;
        }
        
    }
    var jsonString = JsonConvert.SerializeObject(result);
    return jsonString;
})
.WithName("crawlers");

app.MapGet("/locationHistory/{crawlerId}", (int crawlerId, GpsContext gpsContext) =>
{
    var result = new LocationInfo();
    var locations = gpsContext.gps_history
        .Where(u => u.CrawlerId == crawlerId)
        .OrderByDescending(b => b.Timestamp)
        .Take(60)
        .OrderBy(x => x.Timestamp);
    locations.ToList();
    foreach (var data in locations)
    {
        result.GpsHistory.Add(new GpsData(data.Timestamp, data.Latitude, data.Longitude));
    }
    result.currentLatitude = locations?.Last().Latitude ?? 0.0;
    result.currentLongitude = locations?.Last().Longitude ?? 0.0;
    var jsonString = JsonConvert.SerializeObject(result);
    return jsonString;
})
.WithName("locationHistory");

app.MapPost("/addHistory", (HistoryDto payload, GpsContext readings, CrawlerContext crawlersContext) =>
{
    var crawlerFound = crawlersContext.crawlers.SingleOrDefault(u => u.uuid == payload.uuid);
    if (crawlerFound != null)
    {
        foreach (var point in payload.readings)
        {
            var entity = new GpsHistoryEntity
            {
                CrawlerId = crawlerFound!.id,
                Timestamp = point.timestamp,
                Longitude = point.longitude,
                Latitude = point.latitude
            };
            readings.gps_history.Add(entity);
            readings.SaveChanges();
        }
    }

})
.WithName("addHistory");

app.MapPost("/addDiagnostics", (DiagnosticsDto payload, DiagnosticsContext diagnosticsContext, CrawlerContext crawlersContext) =>
{
    var crawlerFound = crawlersContext.crawlers.SingleOrDefault(u => u.uuid == payload.uuid);
    if (crawlerFound != null)
    {
        foreach (var diag in payload.diagnostics)
        {
            var entity = new DiagnosticsHistoryEntity
            {
                CrawlerId = crawlerFound!.id,
                lastLockTimestamp = diag.lastLockTimestamp,
                averageSatelliteCount = diag.averageSatelliteCount,
                averageTryCount = diag.averageTryCount,
                averageWakeupPerLock = diag.averageWakeupPerLock,
                Uptime = diag.uptime
            };
            diagnosticsContext.diagnostics_history.Add(entity);
            diagnosticsContext.SaveChanges();
        }
    }
})
.WithName("addDiagnostics");

app.Run();

public record HistoryDto(string uuid, List<GpsData> readings);

public record GpsData(long timestamp, double longitude, double latitude);
class LocationInfo()
{
    public double currentLatitude;
    public double currentLongitude;
    public List<GpsData> GpsHistory = new List<GpsData>();

}
public record DiagnosticsDto(string uuid, List<Diagnostics> diagnostics);

public record Diagnostics(long lastLockTimestamp, double averageTryCount, double averageSatelliteCount, double averageWakeupPerLock, long uptime);