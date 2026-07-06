using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;
using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

public class GpsHistoryEntity
{
    [Key, DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    [Column("id")]
    public long id { get; set; }
    [Column("timestamp")]
    public long Timestamp { get; set; }

    [Column("crawler_id")]
    public int CrawlerId { get; set; }

    [Column("longitude")]
    public double Longitude { get; set; }

    [Column("latitude")]
    public double Latitude { get; set; }

}




public class GpsContext : DbContext
    {
        public DbSet<GpsHistoryEntity> gps_history { get; set; }

        public string DbPath { get; }

        public GpsContext(DbContextOptions<GpsContext> options)
        : base(options)
        {
        }
    }