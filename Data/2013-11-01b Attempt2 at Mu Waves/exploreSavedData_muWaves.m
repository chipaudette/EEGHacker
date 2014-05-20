
%given_amp_counts = 4.5/2.4e-3;

plot_nchans = 8;
pname2 = 'SavedData\';
ref_activity={};incr_ref_s=[];t_zoom_sec = [];
switch 1
    case 1
        fname2 = 'data_2013-11-01_16-46-54_V1_MuWaves';
        chan_names = {'' '' 'C3' 'C4' '' '' '' ''};
        Iref = []; Itest = [3 4];
        %Iref = []; Itest = [3 4];
        plot_nchans = 4;
        sname = 'Mu Waves, Open BCI V1';
    case 2
        fname2 = 'data_2013-11-01_16-56-00_V2_MuWaves';
        chan_names = {'' '' 'C3' 'C4' '' '' '' ''};
        Iref = []; Itest = [3 4];
        %Iref = []; Itest = [3 4];
        plot_nchans = 4;
        sname = 'Mu Waves, Open BCI V2';
    case 20
        fname2 = 'SecondHandTest_data_2013-10-18_15-31-14.mat';
        chan_names = {'Cz' 'Oz' 'Above C3' 'C3' 'Below C3' '' '' ''};
        Iref = []; Itest = [3 2];
        Iref = []; Itest = [4 2];
        %Iref = []; Itest = [5 2];
        plot_nchans = 5;
        sname = 'Hand Test2';
        
        % Work from end backwards
        incr_ref_s = [0; 
            20+7; 20-7; 27;
            27; 27+1.5+2; 27-1.5+1; 27-1+2+2-2;
            27-2+1-2; 28-1]; % last # based on looking at plot; others from notes
        ref_activity = {'C' 'O' 'T' ...
            'O' 'C' 'O' 'T' ...
            'O' 'C'};
        for I=1:length(ref_activity)
            switch ref_activity{I}
                case 'O'
                    ref_activity{I}='Relax';
                case 'C'
                    ref_activity{I}='Close Eyes';
                case 'M'
                    ref_activity{I}='Move Hand';
                case 'T'
                    ref_activity{I}='Think Hand';
            end
        end
        
        obs_a_start = [52.74; 148.2; 256.5];
        obs_a_end = [76.54; 173.2; 280.2];
    case 30
        fname2 = 'TongueTest_data_2013-10-18_15-44-51';
        chan_names = {'Cz' 'Oz' 'Above C3' 'C3' 'Below C3' '' '' ''};
        Iref = []; Itest = [3 2];
        Iref = []; Itest = [4 2];
        Iref = []; Itest = [5 2];       
        plot_nchans = 5;
        sname = 'Tongue Test';
        
        %t_zoom_sec = [0 266];  %for the actual movement
        t_zoom_sec = [246 523]; %for the thinking about movement
        
        % Work start-to-end
        incr_ref_s = [30;
            25-2; 30-3; 20+2+2+2+3;30-2-2;    
            30-2; 30; 30; 30-2;
            20+2; 30-2; 30-2; 30;
            27;30;27;27;
            25+3;32-3
            ]; % numbers based on notes, except last number based on trying to fit plot
        
        ref_activity = {'O' 'C' 'O' 'M' ...
                        'O' 'C' 'O' 'M' ...
                        'O' 'C' 'O' 'T' ...
                        'O' 'C' 'O' 'T' ...
                        'O' 'C'};
        for I=1:length(ref_activity)
            switch ref_activity{I}
                case 'O'
                    ref_activity{I}='Relax';
                case 'C'
                    ref_activity{I}='Close Eyes';
                case 'M'
                    ref_activity{I}='Move Tongue';
                case 'T'
                    ref_activity{I}='Think Tongue';
            end
        end
                  
        
        
        % Work from beginning forwards
%         first_ref_s = 49.5;
%         incr_ref_s = [30*ones(17,1); 32];
%         times_ref_s = first_ref_s;
%         for i = 2:17
%             times_ref_s(i) = times_ref_s(i-1) + incr_ref_s;
%         end
end

%scale_fac_volts_count=2.23e-8;
scale_fac_volts_per_count = 4.5 / 24 / (2^24);  %full scale voltage / gain / n_bits

filter_rawtraces=1;
notchfilter_rawtraces=1;

%% load data
data2 = load([pname2 fname2]);
fs = data2.fs_Hz;
try
    buff_data_FS = data2.buff_data_FS
catch
    buff_data_FS = data2.buff_data;
end
data2_counts = buff_data_FS * 2^(24-1);  %converts [-1.0 to +1.0] to [-2^23 to +2^23]
data2_V = data2_counts * scale_fac_volts_per_count;

%% filter the data
txt = [];
if (filter_rawtraces)
    disp(['BP Filtering...']);
    bp_Hz = [1 70];
    txt = [txt 'Filterd '];
    [b,a]=butter(2,bp_Hz/(fs/2));
    data2_V = filter(b,a,data2_V);
end

if notchfilter_rawtraces
    disp(['Notch Filtering...']);
    stop_Hz = 60+[-2 2];
    txt = [txt 'Filterd '];
    [b,a]=butter(2,stop_Hz/(fs/2),'stop');
    data2_V = filter(b,a,data2_V);
end

%% analyze data
mean_data2_V = mean(data2_V);
median_data2_V = median(data2_V);
std_data2_V = std(data2_V);
des_precentiles = 0.5+(0.68-0.5)*[-1 1];
spread_data2_V = diff(xpercentile(data2_V,des_precentiles))/2;
spread_data2_V = median(spread_data2_V)*ones(size(spread_data2_V));

%% plot data
%len = min([length(data1_V) length(data2_V) length(data3_V)]);
%len = size(data2_V;
t_sec = ([1:size(data2_V,1)]-1)/fs;

montage_space_uV = 100;
data_montage = data2_V / (1e-6*montage_space_uV);
offsets = [-1:-1:-size(data_montage,2)];
offsets_array = ones(size(data_montage,1),1)*offsets;

%make time-domain montage
[offsets,I]=sort(offsets);chan_txt = {chan_names{I}};
if (0)
    figure;setFigureTallestWidest;
    plot(t_sec,data_montage+offsets_array,'b');
    ylim([min(offsets(1:plot_nchans)) max(offsets(1:plot_nchans))]+[-1 1]);
    %chan_names={};for Ichan=1:size(offsets,2);chan_names{Ichan}=['Chan ' num2str(Ichan)];end;
    set(gca,'YTick',offsets,'YTickLabel',chan_txt);
    xlabel(['Time (sec)']);
    title([txt ' EEG Signals (Spacing = ' num2str(montage_space_uV) ' uV)']);
end

%% make spectrograms
figure;setFigureTallestWidest;
%n_per_page = 3;
%loc1 = [1 2 3];loc2 = [4 5 6];
nrow = ceil(min([plot_nchans size(data2_V)])/2); ncol=2;
count=0;
ax=[];
unique_test_types = unique(ref_activity);
pD_all_chan = {};
for Ichan=1:min([plot_nchans size(data2_V,2)]);
    count=count+1;
    
  
    % do frequency analysis
    N=256*2;overlap = 1-1/(4 * (N / 256));plots=0;
    [pD,wT,freq_Hz]=windowedFFTPlot_spectragram(data2_V(:,Ichan),N,overlap,fs,plots);
    wT = wT + (N/2)/fs;
    
    
    % do analysis specific to Mu waves
    
    %smooth in time;
    ave_sec = 2;
    smooth_pD = pD;
    if (ave_sec > 0)
        block_rate_Hz = mean(1./diff(wT));
        [b,a]=butter(2,(1/ave_sec)/(block_rate_Hz/2));
        smooth_pD = 10.^(filter(b,a,log10(pD)')');
    end
    
    %estimate time of borders between test time periods
    t_marks_sec=[];
    if ~isempty(incr_ref_s)
        t_marks_sec = cumsum(incr_ref_s); t_marks_sec = t_marks_sec - t_marks_sec(end) + size(data2_V,1)/fs;
    end    
   
    % first decide which spectral slices are part of which time period
    t_buff_sec = max([1 ave_sec]);  %exclude this around each transition
    pD_all_test={};
    for Itime = 1:length(t_marks_sec)-1
        inds = find((wT >= t_marks_sec(Itime)+t_buff_sec) & (wT <= t_marks_sec(Itime+1)-t_buff_sec));
        if ~isempty(inds)
            if (0)
                %use raw pD
                pD_this_test = pD(:,inds);
            else
                %use time-smoothed pD
                pD_this_test = smooth_pD(:,inds);
            end
            
            Itest = find(strcmp(unique_test_types,ref_activity{Itime}));
            if (Itest > length(pD_all_test))
                pD_prev = [];
            else
                pD_prev = pD_all_test{Itest};
            end
            pD_new = [pD_prev pD_this_test];
            pD_all_test{Itest}=pD_new;
        end
    end
    pD_all_chan{Ichan}=pD_all_test;
    

        %plot
        subplotTightBorder(nrow,ncol,count);
        imagesc(wT,freq_Hz,10*log10(pD));
        set(gca,'Ydir','normal');
        xlabel('Time (sec)');
        ylabel('Frequency (Hz)');
        %title(tt);
        %cl=get(gca,'Clim');set(gca,'Clim',cl(2)+[-60 0]);
        set(gca,'Clim',-95+[-70 0]);
        %ylim([0 65]);
        ylim([0 30]);
        xlim(t_sec([1 end]));
        cl=get(gca,'Clim');
        %weaText(['Clim = [' num2str(cl(1)) ' ' num2str(cl(2)) '] dB'],1);
        ax(end+1)=gca;
        hold on;
        yl=ylim;
        
        % add markers
        t_marks_sec=[];
        if ~isempty(incr_ref_s)
            t_marks_sec = cumsum(incr_ref_s); t_marks_sec = t_marks_sec - t_marks_sec(end) + size(data2_V,1)/fs;
        end
        for i = length(incr_ref_s):-1:1
            %t_plot = wT(end)-sum(incr_ref_s(i:end));
            plot(t_marks_sec(i)*ones(1,2),ylim,'k--','linewidth',2) % Plot reference times starting from the end and going backwards
            
            i_txt = i-1;
            if ((i_txt > 0) & (i_txt <= length(ref_activity)))
                t_txt = 0.5*(t_marks_sec(i_txt) + t_marks_sec(i_txt+1));
                if (isempty(t_zoom_sec) || ((t_txt >= t_zoom_sec(1)) && (t_txt <= t_zoom_sec(2))))
                    txt = ref_activity{i_txt};
                    h=text(t_txt,yl(2)-0.05*diff(yl),txt,...
                        'HorizontalAlignment','center','verticalalignment','top',...
                        'BackgroundColor','white','FontSize',11);
                    if (length(txt) > 1)
                        set(h,'Rotation',90,'HorizontalAlignment','right','verticalalignment','middle');
                    end
                    if (~strcmpi(txt(1),'O') && ~strcmpi(txt(1),'C') && ~strcmpi(txt(1),'R'))
                        set(h,'FontWeight','Bold','FontSize',11);
                    end
                end
            end
        end
        hold off;
        title([sname ', ' chan_names{Ichan}]);

    
end


linkaxes(ax);

% zoom in, if told to do so
if ~isempty(t_zoom_sec);xlim(t_zoom_sec);end



%% plot frequency analysis of each period
plot_spectra = 0;

yl=[];xl=[];

if (isempty(incr_ref_s)), return, end

if plot_spectra;figure;setFigureTallestWidest;end
nchan = size(data2_V,2);
median_test_uV_sqrtHz=[];
median_uV_per_chan={};
for Ichan = 1:plot_nchans;
    pD_all_test = pD_all_chan{Ichan};
    
    median_uV_per_test=[];
    for Itest = 1:length(unique_test_types)
        V2_per_bin = pD_all_test{Itest};
        Hz_per_bin = fs / ((length(freq_Hz)-1)*2);
        V2_per_Hz = V2_per_bin / Hz_per_bin;
        signal_uV_sqrtHz = sqrt(V2_per_Hz)*1e6;
        
        median_uV_per_test(:,Itest) = median(signal_uV_sqrtHz')';
        
        if plot_spectra
            subplotTightBorder(length(unique_test_types),plot_nchans,(Itest-1)*plot_nchans + Ichan);
            %loglog(freq_Hz,signal_uV_sqrtHz);xlim([1 100]);
            semilogy(freq_Hz,signal_uV_sqrtHz);xlim([0 40]);
            hold on;plot(freq_Hz,median_uV_per_test(:,Itest),'k','linewidth',4);hold off;
            
            if (Itest==length(unique_test_types)); xlabel(['Frequency (Hz)']); end;
            if (Ichan==1);ylabel(['EEG (uV / sqrt(Hz))']);end;
            title([sname ', ' chan_names{Ichan} ', ' unique_test_types{Itest}]);
            %ylim([0.1 100]);
            ylim([0.1 10]);yt = [0.1 1 10];yt_txt = {'0.1' '1' '10'};set(gca,'YTick',yt,'YTickLabel',yt_txt);
            xl=xlim;yl=ylim;
        end
    end
    
    median_uV_per_chan{Ichan} = median_uV_per_test;
end

if ~isempty(yl)
    yl_plots = yl;
    xl_plots = xl;
else
    yl_plots = [0.1 10];
    xl_plots = [0 40];
    yt = [0.1 1 10];
    yt_txt = {'0.1' '1' '10'};        
end

%% plot summary
% for Iplot=1:2;
%     plotCases = 
figure;
setFigureTallestWidest;
ncol=length(median_uV_per_chan); nrow = 3;
for Ichan = 1:length(median_uV_per_chan)
    median_uV_per_test = median_uV_per_chan{Ichan};
    pD_all_test = pD_all_chan{Ichan};

        
    %compare across all frequencies
    subplot(nrow,ncol,Ichan);
    semilogy(freq_Hz,median_uV_per_test,'linewidth',2);
    legend(unique_test_types);
    xlabel('Frequency (Hz)');
    EEG_label =  ['EEG (uV / sqrt(Hz))'];
    ylabel(EEG_label);
    xlim(xl_plots);
    ylim(yl_plots);set(gca,'YTick',yt,'YTickLabel',yt_txt);
    title([sname ', ' chan_names{Ichan}]);

    %prepare classifier
    targ_freq_Hz = 12.21;  
    [foo,Ifreq]=min(abs(freq_Hz - targ_freq_Hz));
    plot_freq_Hz = freq_Hz(Ifreq);
    hold on;yl=ylim;plot(plot_freq_Hz*[1 1],yl,'k--','linewidth',2);hold off;
    
    
    %histogram at one frequency
    bin_uV = 10.^([-1:0.1:1]);
    all_frac=[];
    for Itest = 1:length(unique_test_types)
        V2_per_bin = pD_all_test{Itest};
        V2_per_Hz = V2_per_bin / Hz_per_bin;
        signal_uV_sqrtHz = sqrt(V2_per_Hz)*1e6;

        [n,x_uV]=hist(signal_uV_sqrtHz(Ifreq,:),bin_uV);
        all_frac(:,Itest) = n(:)./sum(n);
    end
    
    
    subplot(nrow,ncol,ncol+Ichan);
    inds = [1:size(all_frac,2)];
    plot_data = all_frac(:,inds);
    h=semilogx(x_uV,plot_data,'linewidth',2);
    set(gca,'XTick',yt,'XTickLabel',yt_txt);
    %if (Iplot==1);h_prev=h;end
    xlabel(EEG_label);
    ylabel(['Fraction of Data']);
    title([sname ', ' chan_names{Ichan} ', Freq = ' num2str(plot_freq_Hz) ' Hz']);
    legend({unique_test_types{inds}});
    ylim([0 0.4]);
    
    %plot cum hist
    subplot(nrow,ncol,2*ncol+Ichan);
    if (size(all_frac,2)==3)
        %hands
        inds = [2 3];
    else
        %tongue
        inds = [3 2]; %actually move the tongue
        %inds = [3 4]; %think about moving the tongue
        
    end
    plot_data = 1-[cumsum(all_frac(:,inds(1))) cumsum(all_frac(:,inds(2)))];
    
    h=semilogx(x_uV,plot_data,'linewidth',2);
    set(gca,'XTick',yt,'XTickLabel',yt_txt);
    %if (Iplot==1);h_prev=h;end
    xlabel(EEG_label);
    ylabel(['Cumulative Fraction of Data']);
    title([sname ', ' chan_names{Ichan} ', Freq = ' num2str(plot_freq_Hz) ' Hz']);
    legend({unique_test_types{inds}});
    ylim([0 1]);yl=ylim;
    
    d_plot_data = abs(diff(plot_data')');
    [foo,Imax]=max(d_plot_data);
    high_val = max([plot_data(Imax,:)]);
    low_val = min([plot_data(Imax,:)]);
    hold on;plot(x_uV(Imax)*[1 1],yl,'k--','linewidth',2);hold off
    weaText({['Ave ' num2str(ave_sec) ' sec'];[num2str(high_val) ', ' num2str(low_val)];[num2str(high_val-low_val)]},4);
end
