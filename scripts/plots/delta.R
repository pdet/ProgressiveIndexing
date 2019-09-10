#devtools::install_github("slowkow/ggrepel")

(function(lp) {
np <- lp[!(lp %in% installed.packages()[,"Package"])]
if(length(np)) install.packages(np,repos=c("http://cran.rstudio.com/"))
x <- lapply(lp,function(x){library(x,character.only=TRUE, quietly=TRUE, warn.conflicts=FALSE)}) 
})(c("dplyr", "ggplot2", "ggthemes", "ggrepel", "data.table", "RColorBrewer"))


theme <- theme_few(base_size = 24)  +  theme(legend.position = "none", plot.title = element_text(size=16, hjust=.5, family="mono"))

dd <- fread("out_delta_first_query.csv")
ORDERING = c("7" = 1,"9" = 2, "11" = 3,"13"= 4)
cols <- c("P. Quick"='#493829',"P. Bucket"='#668d3c',"P. Radix (LSD)"='#cc0000', "P. Radix (MSD)"='#855723')
dd %>% mutate(label=recode(algorithm_id, "7"="P. Quick", "9"="P. Bucket", "11"="P. Radix (LSD)", "13"="P. Radix (MSD)" )) %>% arrange(ORDERING[algorithm_id]) -> pdata

pdf("delta_first_query.pdf", width=6, height=5)
ggplot(pdata, aes(y=total_time, x=fixed_delta, color=label, group=label, shape=label)) + geom_line(size=1.5) +scale_x_log10()+ geom_point(size=5) + theme + ylab("Query Time (s)") + xlab(expression(delta)) + theme(legend.text = element_text(size=28),legend.justification = c(0, 1), legend.position = c(0, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank()) + theme(legend.key.size=unit(1,"cm")) + scale_color_manual(values=cols)
dev.off()


dd <- fread("out_delta_pay_off.csv")

dd %>% mutate(label=recode(algorithm_id, "7"="P. Quick", "9"="P. Bucket", "11"="P. Radix (LSD)", "13"="P. Radix (MSD)" )) %>% arrange(ORDERING[algorithm_id]) -> pdata


pdf("delta_break_even.pdf", width=6, height=5)
ggplot(pdata, aes(y=convergence_nr, x=fixed_delta, color=label, group=label, shape=label)) + geom_line(size=1.5) +scale_x_log10()+ geom_point(size=5) + theme + ylab("Query Number (#)") + xlab(expression(delta))  + theme(legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank())  + theme(legend.text = element_text(size=28),legend.key.size=unit(1,"cm")) + scale_color_manual(values=cols)
dev.off()

dd <- fread("out_delta_sum.csv")

dd %>% mutate(label=recode(algorithm_id, "7"="P. Quick", "9"="P. Bucket", "11"="P. Radix (LSD)", "13"="P. Radix (MSD)" )) %>% arrange(ORDERING[algorithm_id]) -> pdata

pdf("delta_total_time.pdf", width=6, height=5)
ggplot(pdata, aes(y=sum_time, x=fixed_delta, color=label, group=label, shape=label)) + geom_line(size=1.5) +scale_x_log10()+ geom_point(size=5) + theme + ylab("Total Time (s)") + xlab(expression(delta)) + scale_y_continuous(limits=c(0, max(pdata$sum_time)))  + theme(legend.text = element_text(size=28),legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank())  + theme(legend.key.size=unit(1,"cm")) + scale_color_manual(values=cols) 
dev.off()

dd <- fread("out_delta_conv.csv")
dd %>% mutate(label=recode(algorithm_id, "7"="P. Quick", "9"="P. Bucket", "11"="P. Radix (LSD)", "13"="P. Radix (MSD)" )) %>% arrange(ORDERING[algorithm_id]) -> pdata
pdf("delta_convergence.pdf", width=6, height=5)
ggplot(pdata, aes(y=convergence_nr, x=fixed_delta, color=label, group=label, shape=label)) + geom_line(size=1.5) + scale_x_log10()+geom_point(size=5) + theme + ylab("Query Number (#)") + xlab(expression(delta))  + theme(legend.text = element_text(size=28),legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank())  + theme(legend.key.size=unit(1,"cm")) + scale_color_manual(values=cols)
dev.off()

