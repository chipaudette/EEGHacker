
scale_fac = 1;
pname = '2013-11-01a ECG with V2\SavedData\'; scale_fac = 1e5;
pname = '2013-11-01b Attempt2 at Mu Waves\SavedData\';scale_fac = 1e5;
pname = '2013-11-04 Homemade Electrodes\SavedData\'; scale_fac = 1e5;
pname = '2013-11-08 EOG\'; scale_fac = 1e5;
fnames = dir([pname '*.mat']);
for Ifname = 1:length(fnames);
    fname = fnames(Ifname).name;
    
    %load data
    disp(['loading ' pname fname]);
    data = load([pname fname]);
    
    %show the fields
    data
    
    %extract data
    fs = data.fs_Hz;
    if isfield(data,'buff_data');
        units = 'microvolts';
        data = data.buff_data;
        outfname = fname(1:end-4);
        output_precision = '%.1f';
        yl = [-30 30];
    else
        units = 'unknown scale';
        data = data.buff_data_FS*scale_fac;
        outfname = [fname(1:end-4) '_arbitraryScale' num2str(scale_fac)];
        output_precision = '%.1f';
        yl = [];
    end
    
    % plot data
    figure; setFigureTall;ax = [];
    t_sec = ([1:size(data,1)]-1)/fs;
    dec_fac = 4;
    
    subplot(2,1,1);
    plot(t_sec(1:dec_fac:end),data(1:dec_fac:end,:));
    title(fname,'interpreter','none');
    xlabel('Time (sec)');
    ylabel(['EEG (' units ')']);
    ax(end+1)=gca;
    
    subplot(2,1,2);
    bp_Hz = [0.5 40];
    [b,a]=butter(2,bp_Hz/(fs/2));
    fdata = filter(b,a,data);
    notch_Hz = [57 63];
    [b,a]=butter(2,notch_Hz/(fs/2),'stop');
    fdata = filter(b,a,fdata);
    plot(t_sec(1:dec_fac:end),fdata(1:dec_fac:end,:));
    title(['BP Filtered [' num2str(bp_Hz) '] Hz'],'interpreter','none');
    xlabel('Time (sec)');
    ylabel(['EEG (' units ')']);  
    if isempty(yl); foo=fdata(:); std_data = std(foo); I=find(abs(foo) < 2*std_data); std_data = std(foo(I)); yl = std_data*[-3 3];end
    ylim(yl);
    ax(end+1) = gca;
    
    linkaxes(ax,'x');       
    drawnow
    
    %the OpenBCI format needs a counter for the first column...increments 0->255
    sample_ind = rem(([1:size(data,1)]' - 1),256);
    
    %write the data
    outfname = [outfname '.csv'];
    disp(['writing to ' outfname]);
    dlmwrite(outfname,[sample_ind(:) data],'precision',output_precision);

end