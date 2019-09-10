#devtools::install_github("slowkow/ggrepel")
#FULL SCAN = 0.7
(function(lp) {
np <- lp[!(lp %in% installed.packages()[,"Package"])]
if(length(np)) install.packages(np,repos=c("http://cran.rstudio.com/"))
x <- lapply(lp,function(x){library(x,character.only=TRUE, quietly=TRUE, warn.conflicts=FALSE)}) 
})(c("dplyr", "ggplot2", "ggthemes", "ggrepel", "data.table", "RColorBrewer"))

theme <- theme_few(base_size = 24)  +  theme(legend.position = "none", plot.title = element_text(size=16, hjust=.5, family="mono"))

dd <- fread("out_prog_vs_base.csv")
# dd %>%  group_by(algorithm_id,query_number,delta,query_time,indexing_time,total_time) %>%  mutate(query = 1:n(), cumtime=cumsum(total_time)) %>% as.data.frame() -> ddq
dd %>%  group_by(algorithm_id) %>%  mutate(query = 1:n(), cumtime=cumsum(query_time)) %>% as.data.frame() -> ddq

ddq %>% select(algorithm_id,query_number,delta,query_time,indexing_time,total_time, cumtime) %>% group_by(algorithm_id,query_number) %>% summarise_all(funs(median)) -> aggr

# ddq %>% as.data.frame() -> aggr
ORDERING = c("8" = 1,"5"= 5,"15"=15)
cols <- c("P. Quick"='#493829',"P. Stc 10%"='#cc0000',"AA Idx"='#855723')

aggr  -> pdata

pdata %>% mutate(label=recode(algorithm_id, "8"="P. Quick","5"="P. Stc 10%","15"="AA Idx")) -> pdata2

# png("performance_time_prg_vs_bs.png", width = 800, height = 500)
# ggplot(pdata2, aes(y=total_time, x=query_number, color=reorder(label, ORDERING[algorithm_id]), group=reorder(label, ORDERING[algorithm_id]), shape=reorder(label, ORDERING[algorithm_id]))) + geom_line( size=1)+ geom_point(data=pdata4,size=3)  + theme + ylab("Query Time (s)") + xlab("Query (#)") + theme (legend.position = c(0,0))  + guides(fill=guide_legend(nrow=3)) +theme(legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank()) + scale_color_manual(values=cols)
# dev.off()
# pdata2 <- pdata2[sample(nrow(pdata2), 50000), ]
# pdata3<- pdata2[1:10000]
# pdata3 %>% pdata2[sample(10000:158000, 50000), ]
# pdf("performance_time_prg_vs_bs.pdf", width=8, height=5)

# ggplot(pdata2, aes(y=total_time, x=query_number, color=reorder(label, ORDERING[algorithm_id]), group=reorder(label, ORDERING[algorithm_id]), shape=reorder(label, ORDERING[algorithm_id]))) + geom_line( size=1) +scale_x_log10() + geom_point(data=pdata4,size=3)  + theme + ylab("Query Time (log(s))") + xlab("Query (#)") + theme (legend.position = c(0,0)) + geom_hline(yintercept=0.85, color="black", linetype="dashed") + geom_hline(yintercept=0.000003, color="black", linetype="dashed") +  scale_y_log10(breaks=c(0.000003, 0.85), labels=c("Index \n (0.000003)", "Threshold\n (120% Full Scan)"), limits=c(0.000001, 60)) + guides(fill=guide_legend(nrow=3)) +theme(legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank()) + scale_color_manual(values=cols)
# dev.off()

# png("performance_time_prg_vs_bs_cum.png", width = 800, height = 500, res = 300)
# m <-ggplot(pdata2, aes(y=cumtime, x=query_number, color=reorder(label, ORDERING[algorithm_id]), group=reorder(label, ORDERING[algorithm_id]), shape=reorder(label, ORDERING[algorithm_id]))) + geom_line(size=1) + geom_point(data=pdata4,size=3) + theme + ylab("Cumulative Time (log(s))") + xlab("Query (#)") +scale_y_log10() + theme(legend.justification = c(0, 1), legend.position = c(0.65, 0.5), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank()) + scale_color_manual(values=cols)
# print(m)
# dev.off()

pdata2 <- filter(pdata2, query_number < 10000)

pdata4 <- filter (pdata2, query_number == 10)
pdf("performance_time_prg_vs_bs.pdf", width=8, height=5)
ggplot(pdata2, aes(y=total_time, x=query_number, color=reorder(label, ORDERING[algorithm_id]), group=reorder(label, ORDERING[algorithm_id]), shape=reorder(label, ORDERING[algorithm_id]))) + geom_line( size=1) +scale_x_log10() + geom_point(data=pdata4,size=6)  + theme + ylab("Query Time (log(s))") + xlab("Query (#)") + theme (legend.position = c(0,0)) + geom_hline(yintercept=0.9, color="black", linetype="dashed") + geom_hline(yintercept=0.000003, color="black", linetype="dashed") +  scale_y_log10(breaks=c(0.000003, 0.9), labels=c("Index \n (0.000003)", "1.2x Scan\n (0.9)"), limits=c(0.000001, 60)) + guides(fill=guide_legend(nrow=3)) +theme(legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank()) + scale_color_manual(values=cols)
dev.off()
