
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;
using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

public class CrawlerEntity
{
    [Key, DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    [Column("crawler_id")]
    public int id { get; set; }
    [Column("crawler_uuid")]
    public String uuid { get; set; }
    [Column("name")]
    public String? name { get; set; }

}




public class CrawlerContext : DbContext
{
    public DbSet<CrawlerEntity> crawlers { get; set; }

    public string DbPath { get; }

    public CrawlerContext(DbContextOptions<CrawlerContext> options)
    : base(options)
    {
    }
}