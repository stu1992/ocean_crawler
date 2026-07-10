# Stage 1: Build the React application
FROM node:20-alpine AS front-end-build
WORKDIR /app

# Install dependencies first (leverages Docker cache layer)
COPY ./ocean_crawler_frontend/package*.json ./
RUN npm install
RUN npm ci

# Copy source files and build the app
COPY ./ocean_crawler_frontend/public/ ./public/
COPY ./ocean_crawler_frontend/src/ ./src/
RUN npm run build

# Stage 2: Serve the application with Nginx
FROM nginx:alpine AS react_frontend
# Copy the custom Nginx configuration over the default one
COPY nginx.conf /etc/nginx/conf.d/default.conf

# Copy static build files from Stage 1 into the Nginx web root directory
COPY --from=front-end-build /app/build /usr/share/nginx/html

# Expose port 80 for traffic
EXPOSE 80

# Start Nginx in the foreground
CMD ["nginx", "-g", "daemon off;"]


# now build the back end

FROM  mcr.microsoft.com/dotnet/sdk:10.0 AS back-end-build
WORKDIR /App/backend/
COPY ./ocean_crawler_backend/ ./
# Restore as distinct layers
RUN dotnet restore
# Build and publish a release
RUN dotnet publish "ocean_crawler_backend.csproj" -c Release -o /App/publish

FROM mcr.microsoft.com/dotnet/aspnet:10.0 AS dotnet_backend
WORKDIR /App/
COPY --from=back-end-build /App/publish/ .
EXPOSE 8080
ENTRYPOINT ["dotnet", "ocean_crawler_backend.dll"]