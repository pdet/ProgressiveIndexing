(function(lp) {
np <- lp[!(lp %in% installed.packages()[,"Package"])]
if(length(np)) install.packages(np,repos=c("http://cran.rstudio.com/"))
x <- lapply(lp,function(x){library(x,character.only=TRUE, quietly=TRUE, warn.conflicts=FALSE)}) 
})(c("ggplot2", "ggthemes"))


theme <- theme_few(base_size = 24)  +  theme(legend.position = "none", plot.title = element_text(size=16, hjust=.5, family="mono"))


n <- 58562

# rnd <- head(read.csv("random.csv", header=F, sep=";", col.names=c("query", "start")), n)
skew <- head(read.csv("sky_column_dist.csv", header=F, sep=";", col.names=c("query", "start")), n)

png("performance_time_prg_vs_bs.png", width = 800, height = 500)

m <- ggplot(skew, aes(query)) + geom_point(aes(y=start, x=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 360000000), labels=c("0", expression(360000000))) + xlab("Position") + ylab("Value")

print(m)

