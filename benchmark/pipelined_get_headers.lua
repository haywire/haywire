arguments = {}

init = function(args)
    arguments = args
    wrk.headers["X-Small"] = "Small value"
    wrk.headers["X-Medium"] = "Some header with a medium value"
    wrk.headers["X-Large"] = "Some header with a reasonably large value that will take a bit more memory, just like most user agent strings"
end

request = function()
    pipeline_length = tonumber(arguments[1]) or 1
    local r = {}

    for i=1,pipeline_length do
        r[i] = wrk.format("GET", "/")
    end

    req = table.concat(r)

    return req
end
