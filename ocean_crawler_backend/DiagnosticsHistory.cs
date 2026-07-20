using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;
using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

public class DiagnosticsHistoryEntity
{
    [Key, DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    [Column("id")]
    public long id { get; set; }
    [Column("last_lock_timestamp")]
    public long lastLockTimestamp { get; set; }

    [Column("crawler_id")]
    public int CrawlerId { get; set; }

    [Column("uptime")]
    public long Uptime { get; set; }

    [Column("average_try_count")]
    public double averageTryCount { get; set; }

    [Column("average_satellite_count")]
    public double averageSatelliteCount { get; set; }

    [Column("average_wakeup_per_lock")]
    public double averageWakeupPerLock { get; set; }

}




public class DiagnosticsContext : DbContext
    {
        public DbSet<DiagnosticsHistoryEntity> diagnostics_history { get; set; }

        public string DbPath { get; }

        public DiagnosticsContext(DbContextOptions<DiagnosticsContext> options)
        : base(options)
        {
        }
    }