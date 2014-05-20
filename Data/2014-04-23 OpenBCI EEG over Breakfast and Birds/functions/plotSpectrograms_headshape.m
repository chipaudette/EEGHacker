function [mean_cohere,wT,f] = evalCoherence_nearbyOnly(t_sec,data_V,fs,N,overlap,nave,t_analyze_sec,flim);

compare_chan = [2 1;
    3 1;
    4 2;
    5 3;
    6 4;
    7 5;
    8 6;
    8 7;
    ]


fig=figure;setFigureTallestWide;ax=[];
plot_count=0;
noise = 0;
%t_sec = ([1:size(data_V,1)]-1)/fs;
mean_cohere = [];
for Icompare = 1:size(compare_chan,1);
    Ichan1 = compare_chan(Icompare,1);
    Ichan2 = compare_chan(Icompare,2);

    [fftx,wT,f]=windowedFFT2(t_sec,data_V(:,Ichan1),N,overlap,'hanning');
    wT = wT + (N/2)/fs;
    inds = find(f <= fs/2);


    foo = data_V(:,Ichan2) + noise ;
    [ffty,wT,f]=windowedFFT2(t_sec,foo,N,overlap,'hanning');
    wT = wT + (N/2)/fs;
    %nave = round(4*(1/(1 - overlap)));
    [coherence,yx_raw,yx_filt]=calcCoherence(fftx,ffty,nave);

    figure(fig);
    plot_count = plot_count+1;
    nrow=5;ncol=12+1;plotwidth=3;
    %nrow=5;ncol=12;plotwidth=4;
    subplotTightBorder(nrow,ncol,getPosition(ncol,plotwidth,Icompare));
    imagesc(wT,f(inds),coherence(inds,:));
    set(gca,'Ydir','normal');
    if (Ichan2 == Ichan1+1); xlabel('Time (sec)');end
    ylabel('Frequency (Hz)');
    title(['Coherence, Ch ' num2str(Ichan1) ' to Ch ' num2str(Ichan2)]);
    set(gca,'Clim',[0 1]);
    %ylim([0 80]);
    ylim(flim);
    xlim(t_sec([1 end]));
    if ~isempty(t_analyze_sec)
        xlim(t_analyze_sec);
    end
    cl=get(gca,'Clim');
    %weaText(['Clim = [' num2str(cl(1)) ' ' num2str(cl(2)) '] dB'],1);
    ax(end+1)=gca;

    %summarize data
    K=find((wT >= t_analyze_sec(1)) & (wT <= t_analyze_sec(2)));
    mean_cohere(:,Icompare) = nanmedian(coherence(:,K)')';

end

linkaxes(ax);


%% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%5    
function pos = getPosition(ncol,plotwidth,Icompare)
    
%assume comparing side-by-side around the head
%nwide = 2;
nwide = plotwidth;
switch Icompare
    case 1
        %top center
        pos = floor((ncol / 2) + [0:nwide-1]-(nwide/2-1));
    case 2
        %second row, left side, in a smidge
        pos = (2-1)*ncol + [1:nwide] + 1;
    case 3
        %second row, right side, in a smidge
        pos = (2-1)*ncol+ ncol+[-(nwide-1):0] - 1;
    case 4
        %third row, left side
        pos = (3-1)*ncol+ [1:nwide];
    case 5
        %third row, right side
        pos = (3-1)*ncol+ncol+[-(nwide-1):0];
    case 6
        %fourth row, left side, in a smidge
        pos = (4-1)*ncol+ [1:nwide]+1;
    case 7
        %fourth row, right side, in a smidge
        pos = (4-1)*ncol+ncol+[-(nwide-1):0]-1;
    case 8
        %fifth row, center
        pos = (5-1)*ncol+ floor((ncol / 2) + [0:nwide-1]-(nwide/2-1));
end
    
    