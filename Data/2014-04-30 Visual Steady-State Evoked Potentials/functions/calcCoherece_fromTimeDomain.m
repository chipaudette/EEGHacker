function [coherence,wT,f,mean_cohere] = calcCoherece_fromTimeDomain(t_sec,data_V,fs,N,overlap,nave,t_analyze_sec,plots);

data_V = data_V(:,1:2);  %only examine the first two channels


%fig=figure;setFigureTallestWide;ax=[];
%plot_count=0;
noise = 0;
%t_sec = ([1:size(data_V,1)]-1)/fs;
mean_cohere = [];
%for Icompare = 1:size(compare_chan,1);
%    Ichan1 = compare_chan(Icompare,1);
%    Ichan2 = compare_chan(Icompare,2);

Ichan1 = 1;
Ichan2 = 2;

%compute spectra
[fftx,wT,f]=windowedFFT2(t_sec,data_V(:,Ichan1),N,overlap,'hanning');
wT = wT + (N/2)/fs;
inds = find(f <= fs/2);

foo = data_V(:,Ichan2) + noise ;
[ffty,wT,f]=windowedFFT2(t_sec,foo,N,overlap,'hanning');
wT = wT + (N/2)/fs;

%compute coherence
%nave = round(4*(1/(1 - overlap)));
[coherence,yx_raw,yx_filt]=calcCoherence(fftx,ffty,nave);
wT = wT - (0.75*nave*(1-overlap)*N)/fs;  %adjust apparent timing of coherence estimates

if plots

    %     figure(fig);
    %     plot_count = plot_count+1;
    %     nrow=5;ncol=12;plotwidth=3;
    %     %nrow=5;ncol=12;plotwidth=4;
    %     subplotTightBorder(nrow,ncol,getPosition(ncol,plotwidth,Icompare));
    imagesc(wT,f(inds),coherence(inds,:));
    set(gca,'Ydir','normal');
    %if (Icompare >= (size(compare_chan,1)-2)); xlabel('Time (sec)');end
    ylabel('Frequency (Hz)');
    %title(['Coherence, Ch ' num2str(Ichan1) ' to Ch ' num2str(Ichan2)]);
    title('Coherence');
    set(gca,'Clim',[0 1]);
    %ylim([0 80]);
    %ylim(flim);
    %xlim(t_sec([1 end]));
    xlim(t_sec([1 end]));
    %     if ~isempty(t_analyze_sec)
    %         xlim(t_analyze_sec);
    %     end
    %    cl=get(gca,'Clim');
    %weaText(['Clim = [' num2str(cl(1)) ' ' num2str(cl(2)) '] dB'],1);
    %    ax(end+1)=gca;

    if ~isempty(t_analyze_sec);
        for Imark=1:size(t_analyze_sec,1);
            hold on;
            yl=ylim;
            plot(t_analyze_sec(Imark,1)*[1 1],yl,'k--','linewidth',2);
            plot(t_analyze_sec(Imark,2)*[1 1],yl,'k--','linewidth',2);
            hold off
        end
    end
end

%summarize data
mean_cohere=[];
for Itime = 1:size(t_analyze_sec,1);

    if ~isempty(t_analyze_sec)
        K=find((wT >= t_analyze_sec(Itime,1)) & (wT <= t_analyze_sec(Itime,2)));
        mean_cohere(:,Itime) = nanmedian(coherence(:,K)')';
    end
end

%end

%linkaxes(ax);

