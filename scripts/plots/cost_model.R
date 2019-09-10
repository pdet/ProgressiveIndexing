(function(lp) {
np <- lp[!(lp %in% installed.packages()[,"Package"])]
if(length(np)) install.packages(np,repos=c("http://cran.rstudio.com/"))
x <- lapply(lp,function(x){library(x,character.only=TRUE, quietly=TRUE, warn.conflicts=FALSE)}) 
})(c("dplyr", "ggplot2", "ggthemes", "ggrepel"))


datafile = commandArgs(trailingOnly=TRUE)
stopifnot(length(args) == 1)

theme <- theme_few(base_size = 24) + 
theme(axis.title.y=element_text(vjust=0.9), 
  axis.title.x=element_text(vjust=-0.1),
  axis.ticks.x=element_blank(), legend.position = "none")

colors <- c("Cost Model"='#4e6172',"Measured"='#cc0000')


costmodel_files = c('fixed_bucket.csv','fixed_radix_msd.csv','fixed_radix_lsd.csv','fixed_quick.csv','adaptive_bucket.csv','adaptive_radix_msd.csv','adaptive_radix_lsd.csv','adaptive_quick.csv')

costmodel <- costmodel_files[2]

read.table(costmodel, header=T,  sep=",", stringsAsFactors=F) -> loaded_data
df1 <- data.frame(name="Cost Model", time=loaded_data$cost_model_time, query=1:1000)
df2 <- data.frame(name="Measured", time=loaded_data$total_time, query=1:1000)
df3 <- rbind(df2, df1)
pdf( gsub('.csv', '.pdf', costmodel), width=6, height=5)
ggplot(df3, aes(y=time, x=query, color=name, group=name, shape=name)) + geom_line(size=1.5) +scale_y_log10(limits=c(0.000001,10),breaks=c(0.00001, 0.001,0.1,10)) + scale_x_log10(breaks=c(1, 10,100,1000))+ theme + ylab("Time (s)") + xlab("Query (#)") + theme(legend.key.size = unit(0.8, 'cm'),legend.text = element_text(size=28), legend.justification = c(1, 1), legend.position = c(0.58, 0.25), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank()) + scale_color_manual(values=colors)
dev.off()